"""CH32V103 PFIC/TIM2/EXTI state and register tests (not a hardware test)."""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from host_compile import compiler_flags

SDK_ROOT = Path(__file__).resolve().parents[3]
HOST_CC = shutil.which("cc") or shutil.which("gcc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class CH32V103InterruptTest(unittest.TestCase):
    def test_pfic_timer_exti_register_sequences_and_ownership(self):
        workspace = SDK_ROOT / "testdir"
        workspace.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="ch32-irq-host-", dir=workspace) as directory:
            executable = Path(directory) / "interrupt-test"
            command = [HOST_CC, *compiler_flags("c11", strict=True),
                       "-DECOS_CH32V103_INTERRUPT_TEST"]
            for include in (
                "components/core/include", "components/soc/ch32v103/include",
                "hal/interrupt/include", "hal/gpio/include", "hal/gpio_interrupt/include",
                "hal/timer/include", "drivers/interrupt/include", "drivers/gpio/include",
                "drivers/gpio_interrupt/include", "drivers/timer/include",
                "drivers/timer_interrupt/include",
            ):
                command.extend(["-I", str(SDK_ROOT / include)])
            for source in (
                "components/core/src/error.c", "drivers/interrupt/src/interrupt.c",
                "drivers/gpio_interrupt/src/gpio_interrupt.c", "drivers/timer/src/timer.c",
                "drivers/timer_interrupt/src/timer_interrupt.c",
                "components/soc/ch32v103/hal/interrupt/interrupt.c",
                "components/soc/ch32v103/hal/gpio/gpio_interrupt.c",
                "components/soc/ch32v103/hal/timer/timer.c",
                "components/soc/ch32v103/hal/timer/timer_interrupt.c",
                "components/soc/ch32v103/tests/test_interrupts.c",
            ):
                command.append(str(SDK_ROOT / source))
            command.extend(["-o", str(executable)])
            compiled = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            ran = subprocess.run([str(executable)], capture_output=True, text=True, check=False)
            self.assertEqual(ran.returncode, 0, ran.stdout + ran.stderr)
            self.assertIn("PFIC/TIM2/EXTI register tests passed", ran.stdout)


if __name__ == "__main__":
    unittest.main()
