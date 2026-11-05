// Runs the firmware's KeyMatrix (src/keymatrix.cpp) against mocked GPIO and a
// virtual clock, for the adapter co-simulation (cosim.py).
//
//   harness ROWS COLS SETTLE_US ROW_DELAY_US SCANS [READS_FILE]
//
// ROWS / COLS are comma-separated GPIO lists. Prints, one per line:
//   pins P0 P1 ...          input pin of each switch, row by row
//   drive PIN T_US OUT      a row pin changes direction (OUT=1 pulls it low)
//   read N T_US             the Nth column read
//   state SCAN HEXMASK      getState() after each scan
// With READS_FILE (one hex 64-bit GPIO value per line, one per read), each
// read returns that value; without it every read sees all pins high.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "keymatrix.h"

// time one GPIO register access takes on the RP2040 at 125 MHz, roughly
static const double GPIO_CALL_US = 0.05;
static double now_us = 0;
static int reads = 0;
static std::vector<uint64_t> replay;
static uint64_t outputs = 0;

void gpio_init(unsigned) { now_us += GPIO_CALL_US; }
void gpio_pull_up(unsigned) { now_us += GPIO_CALL_US; }
void gpio_put(unsigned, bool) { now_us += GPIO_CALL_US; }
void gpio_set_dir(unsigned pin, bool out) {
    now_us += GPIO_CALL_US;
    const bool was = (outputs >> pin) & 1;
    if (was != out) printf("drive %u %.4f %d\n", pin, now_us, out ? 1 : 0);
    outputs = out ? (outputs | (uint64_t{1} << pin)) : (outputs & ~(uint64_t{1} << pin));
}
uint64_t gpio_get_all64() {
    now_us += GPIO_CALL_US;
    printf("read %d %.4f\n", reads, now_us);
    const uint64_t v = reads < (int)replay.size() ? replay[reads] : ~uint64_t{0};
    reads++;
    return v;
}
void busy_wait_us_32(uint32_t us) { now_us += us; }

static void parsePins(const char* s, int32_t* out, pb_size_t& count) {
    count = 0;
    for (const char* p = s; *p;) {
        out[count++] = strtol(p, (char**)&p, 10);
        if (*p == ',') p++;
    }
}

int main(int argc, char** argv) {
    if (argc < 6) { fprintf(stderr, "usage: harness ROWS COLS SETTLE ROWDELAY SCANS [READS]\n"); return 2; }
    KeyMatrixOptions o = {};
    o.has_enabled = o.enabled = true;
    parsePins(argv[1], o.rowPins, o.rowPins_count);
    parsePins(argv[2], o.colPins, o.colPins_count);
    o.has_settleMicros = true; o.settleMicros = atoi(argv[3]);
    o.has_rowDelayMicros = true; o.rowDelayMicros = atoi(argv[4]);
    const int scans = atoi(argv[5]);
    if (argc > 6) {
        FILE* f = fopen(argv[6], "r");
        unsigned long long v;
        while (f && fscanf(f, "%llx", &v) == 1) replay.push_back(v);
        if (f) fclose(f);
    }
    KeyMatrix& m = KeyMatrix::getInstance();
    m.setup(o);
    printf("pins");
    for (uint32_t i = 0; i < m.getSwitchCount(); i++) printf(" %d", (int)m.getSwitchPin(i));
    printf("\n");
    now_us = 0;  // the scans start at t = 0 in the circuit simulation
    for (int s = 0; s < scans; s++) {
        m.scan();
        printf("state %d %llx\n", s, (unsigned long long)m.getState());
        // the gamepad loop runs between scans; give it a little time
        now_us += 2.0;
    }
    return 0;
}
