// GOW-Port: minimum SCAJ initialization adapted from port/overrides-usa/gow_overrides.cpp.
// Runtime HLE allocations must not overwrite the game's own allocator structures.
#include "game_overrides.h"
#include "runtime/ee_scheduler.h"
#include "ps2_runtime.h"
#include <cstdio>
#include <cstring>
#include "ps2_runtime_macros.h"
#include "ps2x/iop/disc_source.h"
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>
void sub_00185E20_0x185e20(uint8_t *, R5900Context *, PS2Runtime *);
void sub_001262C8_0x1262c8(uint8_t *, R5900Context *, PS2Runtime *);
void sub_001893E0_0x1893e0(uint8_t *, R5900Context *, PS2Runtime *);
void sub_0018B0F8_0x18b0f8(uint8_t *, R5900Context *, PS2Runtime *);
void sub_0018B430_0x18b430(uint8_t *, R5900Context *, PS2Runtime *);
void sub_001890A8_0x1890a8(uint8_t *, R5900Context *, PS2Runtime *);
void sub_001836E8_0x1836e8(uint8_t *, R5900Context *, PS2Runtime *);
void sub_00183590_0x183590(uint8_t *, R5900Context *, PS2Runtime *);
void sub_001B81B8_0x1b81b8(uint8_t *, R5900Context *, PS2Runtime *);
namespace ps2_stubs { void prepareMpegNativeCreate(uint32_t); }
void sub_00270420_0x270420(uint8_t *, R5900Context *, PS2Runtime *);
void gowDumpBranchRing(const char *why);
void sub_0017DB78_0x17db78(uint8_t *, R5900Context *, PS2Runtime *);
namespace {
// USA 17D778 -> GOWS 17DB78: T0 extension 3020A8, previous
// T0 2C6388, frame start 29D754 and elapsed ticks 29D750.
void scajFrameClock(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    static const bool enabled = [] {
        const char *v = std::getenv("GOW_FRAME_AUDIO_CATCHUP");
        return v && v[0] == '1';
    }();
    static const int64_t creditCap = [] { const char *v=std::getenv("GOW_FCLK_CREDIT"); return v ? std::max<int64_t>(0,std::strtoll(v,nullptr,10)) : 150; }();
    static const int64_t debtCap = [] { const char *v=std::getenv("GOW_FCLK_DEBT"); return v ? std::max<int64_t>(0,std::strtoll(v,nullptr,10)) : 600; }();
    static const int64_t maxLag = [] { const char *v=std::getenv("GOW_FCLK_MAXLAG"); return v ? std::max<int64_t>(0,std::strtoll(v,nullptr,10)) : 1500; }();
    static int64_t credit = 0, debt = 0;
    static auto lastWall = std::chrono::steady_clock::now();
    static uint64_t reads = 0, addedCycles = 0;
    static auto window = std::chrono::steady_clock::now();
    if (enabled) {
        const auto wallNow = std::chrono::steady_clock::now();
        const double wallFields = std::chrono::duration<double>(wallNow-lastWall).count()*60.0;
        lastWall = wallNow;
        auto word = [ram](uint32_t a) { uint32_t v; std::memcpy(&v, ram + a, 4); return v; };
        const uint32_t t0 = runtime->memory().eeTimerLoad(0x10000000u) & 0xffffu;
        const uint32_t extended = word(0x3020A8u) + t0 + (t0 < word(0x2C6388u) ? 0x10000u : 0u);
        const uint32_t elapsed = extended - word(0x29D754u);
        constexpr uint32_t capTicks = 36037u; // 3.75 fields, below the game's 4-field clamp
        int64_t need = int64_t(capTicks - std::min(elapsed, capTicks));
        // Keep a small credit from short frames and repay bounded debt from long frames.
        const int64_t wallTicks = std::clamp<int64_t>(llround(wallFields * 9600.0), 0, maxLag > 0 ? maxLag * 96 : 0x7fffffff);
        if (need < 9600) credit = std::min<int64_t>(creditCap, credit + (9600 - need));
        if (need > 9600) debt = std::min<int64_t>(debtCap, debt + (need - 9600));
        const int64_t repay = std::min<int64_t>(debt, std::min<int64_t>(need, 9600));
        debt -= repay;
        need = std::max<int64_t>(0, need - repay);
        need = std::max<int64_t>(0, need - std::min<int64_t>(credit, need));
        credit = std::max<int64_t>(0, credit - std::min<int64_t>(credit, int64_t(9600)));
        if (need > 0) addedCycles += runtime->eeScheduler().catchUpToWall(uint64_t(need) * 512u);
        if (std::getenv("GOW_FCLK_DIAG"))
            std::fprintf(stderr,"[gow-fclk] wall_fields=%.2f wall_ticks=%lld elapsed=%u need=%lld credit=%lld debt=%lld\n",wallFields,(long long)wallTicks,elapsed,(long long)need,(long long)credit,(long long)debt);
    }
    sub_0017DB78_0x17db78(ram, ctx, runtime);
    ++reads;
    const auto now = std::chrono::steady_clock::now();
    if (now - window >= std::chrono::seconds(5)) {
        std::fprintf(stderr, "[scaj-fclk] enabled=%d reads=%llu added_ms=%.1f\n", enabled,
            static_cast<unsigned long long>(reads), double(addedCycles) * 1000.0 / EeScheduler::kEeClockHz);
        reads = addedCycles = 0; window = now;
    }
}
bool scajWideEnabled() {
    static const bool enabled = [] { const char *v=std::getenv("GOW_WIDESCREEN"); return !v || v[0]!='0'; }();
    return enabled;
}
bool scajProgressiveEnabled() {
    static const bool enabled = [] { const char *v=std::getenv("GOW_PROGRESSIVE"); return !v || v[0]!='0'; }();
    return enabled;
}
void scajApplyWideDefaults(uint8_t *ram) {
    if(!scajWideEnabled()) return;
    const uint32_t one=1u, aspect=0x3FE38E39u;
    std::memcpy(ram+0x29DF80u,&one,4);
    for(uint32_t address : {0x32DD30u,0x2FC3E8u,0x2FC3B0u})
        std::memcpy(ram+address,&aspect,4);
}
bool g_scajWideAtFirstCrt=true;
void scajSelectInitialVideoPreset(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    static bool first=true;
    if(first) {
        first=false;
        // 0x220D10 selects 0x2FC3B8 for ProgressiveScan=1 and
        // 0x2FC380 otherwise. Reuse that native preset at boot, avoiding
        // an artificial nested mode switch while the menu is constructing.
        uint32_t presetMode=0;
        std::memcpy(&presetMode,ram+0x2FC3B8u+0x20u,4);
        if(scajProgressiveEnabled() && presetMode==0x50u) {
            SET_GPR_U32(ctx,4,0x2FC3B8u);
            const uint32_t one=1u;
            std::memcpy(ram+0x29DF84u,&one,4);
            std::fprintf(stderr,"[gow-video] initial native ProgressiveScan preset=0x2fc3b8\n");
        } else if(scajProgressiveEnabled()) {
            std::fprintf(stderr,"[gow-video] progressive preset unavailable: mode=%#x; original preset retained\n",presetMode);
        }
    }
    sub_00183590_0x183590(ram,ctx,runtime);
}
void scajInitOptionsMenu(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    static bool first=true;
    if(first) {
        first=false;
        scajApplyWideDefaults(ram);
        if(scajProgressiveEnabled()) {const uint32_t one=1;std::memcpy(ram+0x29DF84u,&one,4);}
        std::fprintf(stderr,"[gow-video] game-option defaults WideScreen=%d ProgressiveScan=%d\n",scajWideEnabled(),scajProgressiveEnabled());
    }
    sub_001B81B8_0x1b81b8(ram,ctx,runtime);
}
void scajSetupCrt(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    if(g_scajWideAtFirstCrt) {scajApplyWideDefaults(ram);g_scajWideAtFirstCrt=false;}
    uint32_t flags=0,mode=0; uint16_t frame=0;
    std::memcpy(&flags,ram+0x32DD34u,4); std::memcpy(&mode,ram+0x32FBF8u,4);
    std::memcpy(&frame,ram+0x336ADCu,2);
    std::fprintf(stderr,"[gow-video] native CRT mode=%#x interlace=%u frame=%u\n",mode,(flags>>1)&1u,frame);
    sub_001836E8_0x1836e8(ram,ctx,runtime);
}
void scajDvdDualInfo(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    auto disc = ps2x::iop::DiscSource::openExtracted(runtime->getIoPaths().cdRoot);
    const uint32_t layer = disc ? disc->layer1Start() : 0u;
    const uint32_t a = GPR_U32(ctx, 4), b = GPR_U32(ctx, 5);
    if (!disc || a > PS2_RAM_SIZE - 4 || b > PS2_RAM_SIZE - 4) {
        SET_GPR_S32(ctx, 2, 0);
    } else {
        const uint32_t dual = layer != 0;
        std::memcpy(ram + a, &dual, 4);
        std::memcpy(ram + b, &layer, 4);
        SET_GPR_S32(ctx, 2, 1);
        std::fprintf(stderr, "[gow-scaj] DVD dual=%u layer1=%u from disc layout\n", dual, layer);
    }
    ctx->pc = GPR_U32(ctx, 31);
}
void scajLoadStartModuleBuffer(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    const uint32_t addr = GPR_U32(ctx, 4), len = GPR_U32(ctx, 5), args = GPR_U32(ctx, 6), result = GPR_U32(ctx, 7);
    if (len > 65536 || args > PS2_RAM_SIZE - len || result > PS2_RAM_SIZE - 4) {
        SET_GPR_S32(ctx, 2, -1);
    } else {
        const auto loaded = runtime->loadIopModuleBuffer(addr, len ? ram + args : nullptr, len);
        if (result) std::memcpy(ram + result, &loaded.startResult, 4);
        SET_GPR_S32(ctx, 2, loaded.handled ? loaded.moduleId : -1);
        std::fprintf(stderr, "[gow-scaj] LoadStartModuleBuffer IOP=%x id=%d start=%d\n", addr, loaded.moduleId, loaded.startResult);
    }
    ctx->pc = GPR_U32(ctx, 31);
}
void scajIoStateDiagnostic(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    if (std::getenv("GOW_SCAJ_IO_DIAG")) {
        static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(10);
        const auto now = std::chrono::steady_clock::now();
        if (now - last > std::chrono::seconds(5)) {
            last = now;
            const auto word = [ram](uint32_t a) { uint32_t v=0; if(a <= PS2_RAM_SIZE-4)std::memcpy(&v,ram+a,4); return v; };
            const uint32_t object = GPR_U32(ctx,17);
            std::fprintf(stderr,"[gow-scaj-io] pc=%x object=%x state=%x result=%x queue=%x ack=%x head=%x current=%x\n",ctx->pc,object,word(object),word(object+4),word(0x32d434),word(0x455000),word(0x29faa8),word(0x29fae8));
            const uint32_t pending=word(0x29faa8), count=word(0x29faac);
            std::fprintf(stderr,"[gow-scaj-io] pending=%x count=%u first=%x last=%x pause=%x queue-index=%x\n",pending,count,word(pending),count<1024?word(pending+4*(count+1)):0,word(0x29d8f8),word(word(0x32d434)+0xc8));
            std::fprintf(stderr,"[gow-scaj-io] ack words:");
            for(uint32_t a=0x455000;a<0x455280;a+=4) { if(word(a)) std::fprintf(stderr," +%x=%x",a-0x455000,word(a)); }
            std::fprintf(stderr,"\n");
        }
    }
    sub_00185E20_0x185e20(ram,ctx,runtime);
}
void scajFilterDiagnostic(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    static unsigned reports = 0;
    if(reports == 0) gowDumpBranchRing("scaj-filter-entry");
    if (ctx->pc != 0x126528u || reports++ < 12) {
        std::fprintf(stderr, "[gow-scaj-filter] pc=%x index=%llx count=%llx sp=%x ra=%x a0=%x a1=%x t7=%x\n",
            ctx->pc, (unsigned long long)GPR_U64(ctx,10), (unsigned long long)GPR_U64(ctx,12),
            GPR_U32(ctx,29), GPR_U32(ctx,31), GPR_U32(ctx,4), GPR_U32(ctx,5), GPR_U32(ctx,15));
    }
    sub_001262C8_0x1262c8(ram,ctx,runtime);
}
uint32_t scajWord(uint8_t *ram,uint32_t a) { uint32_t v=0; if(a<=PS2_RAM_SIZE-4) std::memcpy(&v,ram+a,4); return v; }
void scajNormaliseVideoRing(uint8_t *ram) {
    const uint32_t movie=scajWord(ram,0x303ab8u);
    if(movie && movie<PS2_RAM_SIZE-0x100u) {
        const uint32_t base=scajWord(ram,movie+0xe8),size=scajWord(ram,movie+0xec),write=scajWord(ram,movie+0xf0);
        if(size && base<=PS2_RAM_SIZE && size<=PS2_RAM_SIZE-base && write==base+size) {
            std::memcpy(ram+movie+0xf0,&base,4);
            if(std::getenv("GOW_SCAJ_MOVIE_DIAG")) std::fprintf(stderr,"[scaj-movie] normalized ring end=%x -> base=%x\n",write,base);
        }
    }
}
void scajMovieVideoWrite(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    scajNormaliseVideoRing(ram); sub_0018B0F8_0x18b0f8(ram,ctx,runtime);
}
void scajMovieVideoRead(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    scajNormaliseVideoRing(ram); sub_0018B430_0x18b430(ram,ctx,runtime);
}
// Semantic instruction match: USA 0x188B80 -> SCAJ 0x1893E0.
// Preserve the native stream service while keeping its poll-driven clock at host speed.
void scajMovieStreamRead(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    runtime->eeScheduler().catchUpToHost(EeScheduler::kEeClockHz / 60u);
    if(std::getenv("GOW_SCAJ_MOVIE_DIAG")) {
        static auto last=std::chrono::steady_clock::now()-std::chrono::seconds(5);
        auto now=std::chrono::steady_clock::now();
        if(now-last>=std::chrono::seconds(5)) {
            last=now; const uint32_t movie=scajWord(ram,0x303ab8u);
            std::fprintf(stderr,"[scaj-movie-read] movie=%x pc=%x state=%x pending=%x count=%x stream:",movie,ctx->pc,scajWord(ram,0x29d918),scajWord(ram,0x29faa8),scajWord(ram,0x29faac));
            for(uint32_t off=0;off<0x30;off+=4) std::fprintf(stderr," +%x=%x",off,scajWord(ram,0x2fc890u+off));
            std::fprintf(stderr," movie:");for(uint32_t off=0x48;off<0x70;off+=4)std::fprintf(stderr," +%x=%x",off,scajWord(ram,movie+off));
            std::fprintf(stderr,"\n");
        }
    }
    sub_001890A8_0x1890a8(ram,ctx,runtime);
}
void scajMovieStreamService(uint8_t *ram, R5900Context *ctx, PS2Runtime *runtime) {
    runtime->eeScheduler().catchUpToHost(EeScheduler::kEeClockHz / 60u);
    sub_001893E0_0x1893e0(ram,ctx,runtime);
}
void scajMpegCreate(uint8_t *ram,R5900Context *ctx,PS2Runtime *runtime) {
    ps2_stubs::prepareMpegNativeCreate(GPR_U32(ctx,4));
    sub_00270420_0x270420(ram,ctx,runtime);
}
void applyScajHeapIsolation(PS2Runtime &runtime) {
    runtime.replaceFunction(0x0017DB78u, scajFrameClock);
    runtime.replaceFunction(0x00183590u, scajSelectInitialVideoPreset);
    runtime.replaceFunction(0x001B81B8u, scajInitOptionsMenu);
    runtime.replaceFunction(0x001836E8u, scajSetupCrt);
    std::fprintf(stderr,"[gow-video] defaults widescreen=%d progressive=%d\n",scajWideEnabled(),scajProgressiveEnabled());
    runtime.replaceFunction(0x00270420u, scajMpegCreate);
    runtime.replaceFunction(0x001893E0u, scajMovieStreamService);
    runtime.replaceFunction(0x001890A8u, scajMovieStreamRead);
    runtime.replaceFunction(0x0018B0F8u, scajMovieVideoWrite);
    runtime.replaceFunction(0x0018B430u, scajMovieVideoRead);
    if(std::getenv("GOW_SCAJ_FILTER_DIAG")) { runtime.replaceFunction(0x00126528u, scajFilterDiagnostic); runtime.replaceFunction(0x001262C8u, scajFilterDiagnostic); runtime.replaceFunction(0x0012633Cu, scajFilterDiagnostic); runtime.replaceFunction(0x00126310u, scajFilterDiagnostic); runtime.replaceFunction(0x00126368u, scajFilterDiagnostic); runtime.replaceFunction(0x00126428u, scajFilterDiagnostic); runtime.replaceFunction(0x00126478u, scajFilterDiagnostic); }
    if(std::getenv("GOW_SCAJ_IO_DIAG")) runtime.replaceFunction(0x00185F00u, scajIoStateDiagnostic);
    runtime.replaceFunction(0x00299DA8u, scajLoadStartModuleBuffer);
    runtime.replaceFunction(0x0027BC30u, scajDvdDualInfo);
    // GOW-Port: this SDK entry lies in a preceding delay slot and is absent from generated registration.
    const bool wakeBound = ps2_game_overrides::bindAddressHandler(runtime, 0x00295A50u, "iWakeupThread");
    std::fprintf(stderr, "[gow-scaj] iWakeupThread bound=%d\n", wakeBound);
    runtime.setPrivateGuestHeap(0x000D0000u, 0x000FF000u);
    std::fprintf(stderr, "[gow-scaj] runtime heap isolated below ELF; no guest wait bypasses enabled\n");
}
}
PS2_REGISTER_GAME_OVERRIDE("God of War (SCAJ-973.99 CHS compatibility)", "GOWS_169.41", 0x00100008u, 0u, applyScajHeapIsolation)
