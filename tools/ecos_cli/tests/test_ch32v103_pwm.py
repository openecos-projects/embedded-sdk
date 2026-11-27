"""Exercise the public PWM driver and CH32V103 preload/register behavior on the host."""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from host_compile import compiler_flags

SDK_ROOT = Path(__file__).resolve().parents[3]
HOST_CC = shutil.which("cc") or shutil.which("gcc")


@unittest.skipUnless(HOST_CC, "a host C compiler is required")
class CH32V103PWMTest(unittest.TestCase):
    def test_tim1_and_tim3_outputs(self):
        workspace = SDK_ROOT / "testdir"
        workspace.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="ch32-pwm-", dir=workspace) as directory:
            executable = Path(directory) / "pwm-test"
            command = [HOST_CC, *compiler_flags("c11", strict=True),
                       "-DECOS_CH32V103_INTERRUPT_TEST"]
            for include in ("components/core/include", "components/soc/ch32v103/include",
                            "drivers/pwm/include", "hal/pwm/include"):
                command.extend(["-I", str(SDK_ROOT / include)])
            for source in ("components/core/src/error.c", "drivers/pwm/src/pwm.c",
                           "components/soc/ch32v103/hal/pwm/pwm.c",
                           "components/soc/ch32v103/tests/test_pwm.c"):
                command.append(str(SDK_ROOT / source))
            subprocess.run([*command, "-o", str(executable)], cwd=directory, check=True)
            subprocess.run([str(executable)], cwd=directory, check=True)
