/* Game-side symbols a SETUP HOST link needs, and nothing more.
 *
 * A setup host is the build the release zip carries: the same executable with
 * no recompiled code at all (runner.cmake, snesrecomp_target_generated_code).
 * The framework resolves its own half of that link in
 * snesrecomp/runner/src/setup_host_dispatch.c; these eight are the game's
 * half -- entry points src/zelda_rtl.c calls directly, which normally come
 * from src/gen/.
 *
 * This is NOT a stub in the sense recomp-ai-rules forbids, and the test is
 * reachability rather than intent: SnesInit() calls Die() before any guest
 * code runs when SNESRECOMP_SETUP_HOST is defined (common_cpu_infra.c), so
 * there is no execution path in this binary that reaches a line below. The
 * only path forward here is the launcher's Generate & rebuild wizard, which
 * replaces the binary with one that has the real bodies.
 *
 * They abort rather than return a value for exactly that reason. A stub that
 * returned RECOMP_RETURN_NORMAL would be inventing a result to get past
 * uncovered code, and if the reachability argument above ever stopped holding
 * it would turn a loud failure into a silent wrong one. Reached = the guard
 * in SnesInit is broken, and that is worth a crash with a name attached.
 */

#include "cpu_state.h"

#if !defined(SNESRECOMP_SETUP_HOST)
#error "zelda_setup_host_stubs.c is only compiled into SNESRECOMP_SETUP_HOST builds"
#endif

extern void Die(const char *error);

static void zelda_setup_host_unreachable(const char *name)
{
    /* One message, the symbol's own name, and no return. */
    static char msg[256];
    snprintf(msg, sizeof(msg),
             "setup build: recompiled entry point '%s' was called, but this "
             "binary has no game code. Use the launcher's \"Generate & "
             "rebuild\" with your own ROM; the rebuilt executable is the "
             "playable one.", name);
    Die(msg);
}

#define ZELDA_SETUP_HOST_VOID(fn)                                             \
    void fn(CpuState *cpu)                                                    \
    {                                                                         \
        (void)cpu;                                                            \
        zelda_setup_host_unreachable(#fn);                                    \
    }

#define ZELDA_SETUP_HOST_RECOMP(fn)                                           \
    RecompReturn fn(CpuState *cpu)                                            \
    {                                                                         \
        (void)cpu;                                                            \
        zelda_setup_host_unreachable(#fn);                                    \
        return (RecompReturn)0;      /* not reached; Die() does not return */ \
    }

/* CPU vectors, driven from zelda_rtl.c's frame loop. */
ZELDA_SETUP_HOST_VOID(I_RESET)
ZELDA_SETUP_HOST_VOID(I_IRQ)
ZELDA_SETUP_HOST_VOID(Interrupt_NMI)

/* Mode 7 polyhedral routines (the triforce intro), called directly rather
 * than reached through the dispatch table. */
ZELDA_SETUP_HOST_RECOMP(Polyhedral_SetShapePointer_M1X1)
ZELDA_SETUP_HOST_RECOMP(Polyhedral_SetRotationMatrix_M1X1)
ZELDA_SETUP_HOST_RECOMP(Polyhedral_OperateRotation_M1X1)
ZELDA_SETUP_HOST_RECOMP(Polyhedral_EmptyBitMapBuffer_M1X1)
ZELDA_SETUP_HOST_RECOMP(Polyhedral_DrawPolyhedron_M1X1)
