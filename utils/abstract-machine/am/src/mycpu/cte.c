#include <am.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->cause) {
      case 0x8000000000000007:
        ev.event = EVENT_IRQ_TIMER; break;
      case 11:
        if (c->GPR1 == -1) {
          ev.event = EVENT_YIELD;
          c->epc += 4;
        } else {
          ev.event = EVENT_SYSCALL;
        }
        break;
      default:
        ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
  asm volatile("li a7, -1; ecall");
}

bool ienabled() {
  return false;
}

// interrupt set, just for interrupt test
void iset(bool enable) {
  uint64_t mie_mtie = 0x080;
  if (enable) {
    asm volatile("csrsi mstatus, 8");
    asm volatile("csrs mie, %0" : : "r"(mie_mtie));
  } else {
    asm volatile("csrci mstatus, 8");
    asm volatile("csrc mie, %0" : : "r"(mie_mtie));
  }
}
