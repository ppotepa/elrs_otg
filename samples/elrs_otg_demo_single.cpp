// elrs_otg_demo_single.cpp
//
// Windows-only, C++17, single-file demonstration of:
//  - Opening a USB CDC COM port (ExpressLRS TX) with Win32
//  - Reading + framing MSP v1 and CRSF packets concurrently
//  - Parsing a "link stats + spectrum" MSP payload (func 0x2D placeholder)
//  - Thread-safe RadioState with metrics + spectrum bins + trend buffers
//  - 500 ms polling loop for auto-telemetry
//
// Build (example):
//   cl /std:c++17 /EHsc elrs_otg_demo_single.cpp
//
// Usage:
//   elrs_otg_demo_single.exe COM5
//
// Notes:
//   * MSP function 0x2D and payload layout below match YOUR current handler.
//     If your firmware uses a different ID/format, adjust in TelemetryHandler::onMspFrame_LinkStats().
//   * CRSF parsing is implemented and logged; integrate known types as needed.
//   * This is a teaching scaffold—factor into your UsbBridge, TelemetryHandler, RadioState, MspCommands.

#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <cfgmgr32.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <array>
#include <deque>

// ============ Utilities ============
static inline uint64_t now_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// ============ RadioState ============
struct RadioState
{
    // Scalars
    std::atomic<int> rssi{-127};
    std::atomic<int> lq{0};
    std::atomic<int> snr{-30};
    std::atomic<int> tx_power_dbm{0};
    std::atomic<uint64_t> last_stats_ms{0};

    // Spectrum (shared, guarded)
    std::mutex mtx;
    std::vector<uint8_t> spectrum_bins; // raw power/bucket value per bin
    uint64_t spectrum_ts_ms{0};

    // Trends (fixed-size deques)
    std::deque<int> rssi_hist;
    std::deque<int> lq_hist;
    std::deque<int> txp_hist;
    static constexpr size_t kTrendLen = 120; // ~1 min at 500ms

    void push_trends(int rssi_v, int lq_v, int txp_v)
    {
        auto push_capped = [](std::deque<int> &dq, int v, size_t N)
        {
            dq.push_back(v);
            if (dq.size() > N)
                dq.pop_front();
        };
        std::lock_guard<std::mutex> lk(mtx);
        push_capped(rssi_hist, rssi_v, kTrendLen);
        push_capped(lq_hist, lq_v, kTrendLen);
        push_capped(txp_hist, txp_v, kTrendLen);
    }
};

static RadioState g_state;

// ============ SerialPort (Win32) ============
class SerialPort
{
public:
    SerialPort() = default;
    ~SerialPort() { close(); }

    bool open(const std::wstring &comName, DWORD baud = 400000)
    {
        close();
        std::wstring path = L"\\\\.\\" + comName; // e.g., COM5 -> \\\\.\\COM5
        h_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h_ == INVALID_HANDLE_VALUE)
        {
            std::wcerr << L"[Serial] CreateFile failed for " << path << L", err=" << GetLastError() << L"\n";
            return false;
        }

        // Configure timeouts: quick-ish reads
        COMMTIMEOUTS to{};
        to.ReadIntervalTimeout = 50;
        to.ReadTotalTimeoutMultiplier = 0;
        to.ReadTotalTimeoutConstant = 50;
        to.WriteTotalTimeoutMultiplier = 0;
        to.WriteTotalTimeoutConstant = 50;
        SetCommTimeouts(h_, &to);

        // Configure DCB
        DCB dcb{};
        dcb.DCBlength = sizeof(DCB);
        if (!GetCommState(h_, &dcb))
        {
            std::cerr << "[Serial] GetCommState failed.\n";
            close();
            return false;
        }
        dcb.BaudRate = baud;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        if (!SetCommState(h_, &dcb))
        {
            std::cerr << "[Serial] SetCommState failed.\n";
            close();
            return false;
        }

        // Purge buffers
        PurgeComm(h_, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
        std::wcout << L"[Serial] Opened " << comName << L" @ " << baud << L" baud\n";
        return true;
    }

    void close()
    {
        if (isOpen())
        {
            CloseHandle(h_);
            h_ = INVALID_HANDLE_VALUE;
        }
    }

    bool isOpen() const { return h_ != INVALID_HANDLE_VALUE; }

    // Blocking-ish read with configured timeouts
    size_t read(uint8_t *buf, size_t cap)
    {
        if (!isOpen())
            return 0;
        DWORD n = 0;
        if (!ReadFile(h_, buf, (DWORD)cap, &n, nullptr))
            return 0;
        return (size_t)n;
    }

    bool writeAll(const uint8_t *data, size_t len)
    {
        if (!isOpen())
            return false;
        DWORD written = 0;
        if (!WriteFile(h_, data, (DWORD)len, &written, nullptr))
            return false;
        return written == len;
    }

private:
    HANDLE h_ = INVALID_HANDLE_VALUE;
};

// ============ MSP v1 Deframer ============
class MspDeframer
{
public:
    using MspCallback = std::function<void(uint8_t func, bool from_device, const std::vector<uint8_t> &payload)>;

    explicit MspDeframer(MspCallback cb) : cb_(std::move(cb)) {}

    void feed(uint8_t b)
    {
        switch (st_)
        {
        case S0:
            if (b == '$')
                st_ = S1;
            else
                reset();
            break;
        case S1:
            if (b == 'M')
                st_ = S2;
            else
                reset();
            break;
        case S2:
            if (b == '<' || b == '>')
            {
                dir_from_device_ = (b == '>');
                st_ = S_LEN;
            }
            else
                reset();
            break;
        case S_LEN:
            len_ = b;
            sum_ = b;
            payload_.clear();
            st_ = S_FUNC;
            break;
        case S_FUNC:
            func_ = b;
            sum_ ^= b;
            if (len_ == 0)
                st_ = S_CHK;
            else
                st_ = S_PAY;
            break;
        case S_PAY:
            payload_.push_back(b);
            sum_ ^= b;
            if (payload_.size() >= len_)
                st_ = S_CHK;
            break;
        case S_CHK:
            chk_ = b;
            if (chk_ == sum_)
            {
                if (cb_)
                    cb_(func_, dir_from_device_, payload_);
            }
            reset();
            break;
        }
    }

private:
    enum State
    {
        S0,
        S1,
        S2,
        S_LEN,
        S_FUNC,
        S_PAY,
        S_CHK
    } st_ = S0;
    bool dir_from_device_ = false;
    uint8_t len_ = 0, func_ = 0, sum_ = 0, chk_ = 0;
    std::vector<uint8_t> payload_;
    void reset()
    {
        st_ = S0;
        dir_from_device_ = false;
        len_ = func_ = sum_ = chk_ = 0;
        payload_.clear();
    }
};

// ============ CRSF Deframer ============
class CrsfDeframer
{
public:
    using CrsfCallback = std::function<void(uint8_t addr, uint8_t type, const std::vector<uint8_t> &payload)>;

    explicit CrsfDeframer(CrsfCallback cb) : cb_(std::move(cb)) {}

    void feed(uint8_t b)
    {
        switch (st_)
        {
        case A_WAIT:
            addr_ = b;
            st_ = L_WAIT;
            break;
        case L_WAIT:
            len_ = b; // length = type + payload + crc
            if (len_ < 2)
            {
                reset();
                break;
            }
            buf_.clear();
            cnt_ = 0;
            st_ = BODY;
            break;
        case BODY:
            buf_.push_back(b);
            if (++cnt_ >= len_)
            {
                // buf_ layout: [type][payload...][crc]
                if (buf_.size() >= 2)
                {
                    uint8_t type = buf_[0];
                    uint8_t crc = buf_.back();
                    // compute CRC8 DVB-S2 over [type + payload]
                    uint8_t calc = crc8_dvb_s2(buf_.data(), (uint32_t)buf_.size() - 1);
                    if (calc == crc)
                    {
                        std::vector<uint8_t> payload(buf_.begin() + 1, buf_.end() - 1);
                        if (cb_)
                            cb_(addr_, type, payload);
                    }
                }
                reset();
            }
            break;
        }
    }

private:
    enum State
    {
        A_WAIT,
        L_WAIT,
        BODY
    } st_ = A_WAIT;
    uint8_t addr_ = 0, len_ = 0;
    std::vector<uint8_t> buf_;
    uint8_t cnt_ = 0;

    // CRC-8 DVB-S2 (poly 0xD5, init 0x00, refin/refout=false)
    static uint8_t crc8_dvb_s2(const uint8_t *data, uint32_t len)
    {
        uint8_t crc = 0x00;
        for (uint32_t i = 0; i < len; ++i)
        {
            crc ^= data[i];
            for (int k = 0; k < 8; ++k)
            {
                if (crc & 0x80)
                    crc = (crc << 1) ^ 0xD5;
                else
                    crc <<= 1;
            }
        }
        return crc;
    }

    void reset()
    {
        st_ = A_WAIT;
        addr_ = len_ = cnt_ = 0;
        buf_.clear();
    }
};

// ============ TelemetryHandler ============
class TelemetryHandler
{
public:
    void onMspFrame(uint8_t func, bool from_device, const std::vector<uint8_t> &payload)
    {
        // Only process frames coming FROM the device (dir '>')
        if (!from_device)
            return;

        // Example: your current flow uses MSP function 0x2D for link stats + optional spectrum extension
        if (func == 0x2D)
        {
            onMspFrame_LinkStats(payload);
        }
        else
        {
            // Unhandled MSP func; debug log:
            // std::cout << "[MSP] func=0x" << std::hex << int(func) << " len=" << std::dec << payload.size() << "\n";
        }
    }

    void onCrsfFrame(uint8_t addr, uint8_t type, const std::vector<uint8_t> &payload)
    {
        // Example hook: 0x14 is commonly LINK_STATISTICS in CRSF.
        // You can decode here if you want CRSF-side parsing as well.
        // For now, just basic tracing for visibility:
        (void)addr;
        (void)type;
        (void)payload;
        // std::cout << "[CRSF] addr=0x" << std::hex << int(addr) << " type=0x" << int(type)
        //           << " len=" << std::dec << payload.size() << "\n";
    }

private:
    void onMspFrame_LinkStats(const std::vector<uint8_t> &p)
    {
        // Example payload layout (align with your firmware/handler):
        // [0]=RSSI (int8 or uint8)
        // [1]=LQ   (0..100)
        // [2]=SNR  (int8)
        // [3]=TXP  (dBm, uint8)
        // [4..]=Optional spectrum bins (N bytes)
        if (p.size() < 4)
            return;

        int rssi = (int8_t)p[0];
        int lq = (int)p[1];
        int snr = (int8_t)p[2];
        int txp = (int)p[3];
        g_state.rssi.store(rssi);
        g_state.lq.store(lq);
        g_state.snr.store(snr);
        g_state.tx_power_dbm.store(txp);
        g_state.last_stats_ms.store(now_ms());
        g_state.push_trends(rssi, lq, txp);

        if (p.size() > 4)
        {
            std::lock_guard<std::mutex> lk(g_state.mtx);
            g_state.spectrum_bins.assign(p.begin() + 4, p.end());
            g_state.spectrum_ts_ms = now_ms();
        }

        // Minimal console feedback to prove life:
        std::cout << "[MSP] RSSI=" << rssi << " dBm, LQ=" << lq
                  << ", SNR=" << snr << " dB, TX=" << txp << " dBm"
                  << " | spectrum=" << (p.size() > 4 ? (p.size() - 4) : 0) << " bins\n";
    }
};

// ============ MSP Commands ============
class MspCommands
{
public:
    explicit MspCommands(SerialPort &sp) : sp_(sp) {}

    // Sends MSP v1 request: func id + optional payload
    bool sendRequest(uint8_t func, const std::vector<uint8_t> &data = {})
    {
        // $ M < [len] [func] [payload bytes...] [chk=xor(len^func^payload)]
        std::vector<uint8_t> pkt;
        pkt.reserve(5 + data.size());
        pkt.push_back('$');
        pkt.push_back('M');
        pkt.push_back('<');
        uint8_t len = (uint8_t)data.size();
        pkt.push_back(len);
        uint8_t chk = len;
        pkt.push_back(func);
        chk ^= func;
        for (uint8_t b : data)
        {
            pkt.push_back(b);
            chk ^= b;
        }
        pkt.push_back(chk);
        return sp_.writeAll(pkt.data(), pkt.size());
    }

    // Example: request link stats with optional "with spectrum" feature flag.
    // You can define a one-byte flags payload where bit0=include spectrum.
    bool requestLinkStats(bool withSpectrum)
    {
        const uint8_t kFuncLinkStats = 0x2D; // placeholder: matches your current handler choice
        uint8_t flags = withSpectrum ? 0x01 : 0x00;
        return sendRequest(kFuncLinkStats, std::vector<uint8_t>{flags});
    }

private:
    SerialPort &sp_;
};

// ============ UsbBridge (reader thread) ============
class UsbBridge
{
public:
    using ByteSink = std::function<void(const uint8_t *, size_t)>;

    explicit UsbBridge(SerialPort &sp) : sp_(sp) {}

    void start()
    {
        stop_.store(false);
        th_ = std::thread([this]
                          { run(); });
    }
    void stop()
    {
        stop_.store(true);
        if (th_.joinable())
            th_.join();
    }

    void setByteSink(const ByteSink &sink) { sink_ = sink; }

private:
    void run()
    {
        std::array<uint8_t, 4096> buf{};
        while (!stop_.load())
        {
            size_t n = sp_.read(buf.data(), buf.size());
            if (n == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (sink_)
                sink_(buf.data(), n);
        }
    }

    SerialPort &sp_;
    std::atomic<bool> stop_{true};
    std::thread th_;
    ByteSink sink_;
};

// ============ App glue ============
int wmain(int argc, wchar_t **argv)
{
    if (argc < 2)
    {
        std::wcerr << L"Usage: " << argv[0] << L" COMx\n";
        return 1;
    }
    std::wstring com = argv[1];

    SerialPort sp;
    if (!sp.open(com, 400000))
    { // use 400k to comfortably move bigger payloads
        return 2;
    }

    TelemetryHandler telemetry;
    // MSP deframer → TelemetryHandler
    MspDeframer msp_df([&](uint8_t func, bool from_dev, const std::vector<uint8_t> &pay)
                       { telemetry.onMspFrame(func, from_dev, pay); });
    // CRSF deframer → TelemetryHandler (optional)
    CrsfDeframer crsf_df([&](uint8_t addr, uint8_t type, const std::vector<uint8_t> &pay)
                         { telemetry.onCrsfFrame(addr, type, pay); });

    // Byte router: feed both deframers; they ignore non-matching byte streams naturally
    UsbBridge bridge(sp);
    bridge.setByteSink([&](const uint8_t *p, size_t n)
                       {
    for (size_t i = 0; i < n; ++i) {
      uint8_t b = p[i];
      msp_df.feed(b);
      crsf_df.feed(b);
    } });
    bridge.start();

    MspCommands msp(sp);

    // Polling thread: request link stats (+spectrum) every ~500 ms
    std::atomic<bool> stop{false};
    std::thread poller([&]
                       {
    const bool request_spectrum = true; // toggle as needed
    while (!stop.load()) {
      bool ok = msp.requestLinkStats(request_spectrum);
      if (!ok) std::cerr << "[MSP] requestLinkStats write failed\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    } });

    // Console HUD loop (placeholder for FTXUI redraw trigger)
    std::cout << "Running. Press Ctrl+C to quit.\n";
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        int rssi = g_state.rssi.load();
        int lq = g_state.lq.load();
        int snr = g_state.snr.load();
        int txp = g_state.tx_power_dbm.load();
        uint64_t stats_age = now_ms() - g_state.last_stats_ms.load();

        size_t bins = 0;
        uint64_t spec_age = 0;
        {
            std::lock_guard<std::mutex> lk(g_state.mtx);
            bins = g_state.spectrum_bins.size();
            spec_age = g_state.spectrum_ts_ms ? (now_ms() - g_state.spectrum_ts_ms) : 0;
        }

        std::cout << "[HUD] RSSI=" << rssi << " LQ=" << lq << " SNR=" << snr
                  << " TX=" << txp << " | stats_age=" << stats_age << "ms"
                  << " | spectrum_bins=" << bins << " age=" << spec_age << "ms\n";
    }

    // Never reached in this demo:
    stop.store(true);
    poller.join();
    bridge.stop();
    sp.close();
    return 0;
}
