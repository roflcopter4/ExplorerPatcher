#pragma once
#ifndef _H_HOOKING_H_
#define _H_HOOKING_H_

#define STRAT_REPLACE_ANY_TYPE_OF_JUMP_WITH_NOP         0
#define STRAT_REPLACE_ANY_TYPE_OF_JUMP_WITH_ALWAYS_JUMP 1
#define HOOK_WITH_FUNCHOOK 0
#define HOOK_WITH_DETOURS  1
#define HOW_TO_HOOK        0

#if HOW_TO_HOOK == HOOK_WITH_FUNCHOOK
# ifdef _M_ARM64
#  error Cannot compile for ARM64 using funchook. Change the source to hook with Detours and try again. Compilation aborted.
# endif
# include <distorm.h>
# include <funchook.h>
# pragma comment(lib, "funchook.lib")
# pragma comment(lib, "Psapi.lib") // required by funchook
# pragma comment(lib, "distorm.lib")
funchook_t *funchook = 0;
#elif HOW_TO_HOOK == HOOK_WITH_DETOURS
# include <detours.h>
# include <processthreadsapi.h>
# pragma comment(lib, "detours.lib")
# ifndef _M_ARM64
#  include <distorm.h>
#  pragma comment(lib, "distorm.lib")
# endif
void *
funchook_create(void)
{
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());
      return (void *)1;
}
int
funchook_uninstall(void *_this, int flags)
{
      return 0;
}
int
funchook_destroy(void *_this)
{
      return 0;
}
int
funchook_prepare(void *funchook, void **target_func, void *hook_func)
{
      return DetourAttach(target_func, hook_func);
}
int
funchook_install(void *funchook, int flags)
{
      return DetourTransactionCommit();
}

void *funchook = 0;
#else
# error "Must hook with either Detours or FuncHook."
#endif

#endif