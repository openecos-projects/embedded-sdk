import sys
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.configuration import _board_resources_header  # noqa: E402
from ecos_cli.project_model import ManifestValidationError, _soft_spi_resource  # noqa: E402


class SoftSpiBoardResourceTest(unittest.TestCase):
    def setUp(self):
        self.resource = {
            "driver": "soft-spi",
            "chip_select": {"controller": 0, "pin": 0},
            "clock": {"controller": 0, "pin": 1},
            "mosi": {"controller": 0, "pin": 2},
            "miso": {"controller": 0, "pin": 3},
            "mode": 3,
        }

    def test_generates_public_pin_and_mode_macros(self):
        header = _board_resources_header(
            {"resources": {"soft-spi-bus": self.resource}},
            {"soft-spi-bus"},
        )
        self.assertIn("#define ECOS_BOARD_HAS_SOFT_SPI_BUS 1", header)
        self.assertIn('#include "ecos/driver/gpio.h"', header)
        self.assertNotIn('#include "ecos/soft_spi.h"', header)
        self.assertIn(
            "#define ECOS_BOARD_SOFT_SPI_CHIP_SELECT \\\n    { ECOS_GPIO_PORT_0, 0u }",
            header,
        )
        self.assertIn("#define ECOS_BOARD_SOFT_SPI_MODE 3u", header)
        self.assertNotIn("ECOS_BOARD_HAS_QSPI_BUS 1", header)

    def test_rejects_duplicate_pins(self):
        duplicate = dict(self.resource)
        duplicate["miso"] = duplicate["clock"]
        with self.assertRaisesRegex(ManifestValidationError, "must be distinct"):
            _soft_spi_resource(duplicate, Path("boards/test.yml"))

    def test_rejects_pin_outside_gpio_width(self):
        invalid = dict(self.resource)
        invalid["miso"] = {"controller": 0, "pin": 32}
        with self.assertRaisesRegex(ManifestValidationError, "between 0 and 31"):
            _soft_spi_resource(invalid, Path("boards/test.yml"))


if __name__ == "__main__":
    unittest.main()
