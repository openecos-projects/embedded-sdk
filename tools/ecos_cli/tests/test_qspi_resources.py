import sys
import unittest
from pathlib import Path


SOURCE_ROOT = Path(__file__).parents[1] / "src"
sys.path.insert(0, str(SOURCE_ROOT))

from ecos_cli.configuration import _board_resources_header  # noqa: E402
from ecos_cli.project_model import (  # noqa: E402
    ManifestValidationError,
    _display_resource,
)


class QSPIBoardResourceHeaderTest(unittest.TestCase):
    def test_generates_qspi_and_st7735_resource_contract(self):
        header = _board_resources_header(
            {
                "resources": {
                    "qspi-bus": {
                        "controller": 0,
                        "clock_divider": 3,
                    },
                    "display": {
                        "chip_select": 0,
                        "dc_gpio": {"controller": 0, "pin": 29},
                        "reset_gpio": {"controller": 0, "pin": 30},
                        "backlight_gpio": {"controller": 0, "pin": 31},
                        "width": 128,
                        "height": 128,
                        "rotation": 0,
                        "horizontal_offset": 0,
                        "vertical_offset": 0,
                    },
                }
            }
        )
        self.assertIn("#define ECOS_BOARD_HAS_QSPI_BUS 1", header)
        self.assertIn(
            "#define ECOS_BOARD_QSPI_BUS_CONTROLLER ((ecos_qspi_id_t)0u)",
            header,
        )
        self.assertIn("#define ECOS_BOARD_QSPI_BUS_CLOCK_DIVIDER 3u", header)
        self.assertIn("#define ECOS_BOARD_HAS_DISPLAY 1", header)
        self.assertIn("#define ECOS_BOARD_DISPLAY_CHIP_SELECT ECOS_QSPI_CS_0", header)
        self.assertIn("#define ECOS_BOARD_DISPLAY_DC_PORT ECOS_GPIO_PORT_0", header)
        self.assertIn("#define ECOS_BOARD_DISPLAY_DC_PIN 29u", header)
        self.assertIn("#define ECOS_BOARD_DISPLAY_RESET_PORT ECOS_GPIO_PORT_0", header)
        self.assertIn("#define ECOS_BOARD_DISPLAY_RESET_PIN 30u", header)
        self.assertIn(
            "#define ECOS_BOARD_DISPLAY_BACKLIGHT_PORT ECOS_GPIO_PORT_0", header
        )
        self.assertIn("#define ECOS_BOARD_DISPLAY_BACKLIGHT_PIN 31u", header)

    def test_omits_unrequested_resources(self):
        board = {
            "resources": {
                "gpio-demo": {
                    "input": {
                        "controller": 1,
                        "pin": 7,
                        "label": "GPIO1[7]",
                        "idle_level": "high",
                    },
                    "output": {
                        "controller": 1,
                        "pin": 5,
                        "label": "GPIO1[5]",
                        "initial_level": "high",
                    },
                },
                "qspi-bus": {
                    "controller": 0,
                    "clock_divider": 3,
                },
                "display": {
                    "chip_select": 0,
                    "dc_gpio": {"controller": 0, "pin": 29},
                    "reset_gpio": {"controller": 0, "pin": 30},
                    "backlight_gpio": {"controller": 0, "pin": 31},
                    "width": 128,
                    "height": 128,
                    "rotation": 0,
                    "horizontal_offset": 0,
                    "vertical_offset": 0,
                },
            }
        }
        header = _board_resources_header(board, set())
        self.assertIn("#define ECOS_BOARD_HAS_GPIO_DEMO 0", header)
        self.assertIn("#define ECOS_BOARD_HAS_QSPI_BUS 0", header)
        self.assertIn("#define ECOS_BOARD_HAS_DISPLAY 0", header)
        self.assertNotIn("#include", header)

        header = _board_resources_header(board, {"display"})
        self.assertIn("#define ECOS_BOARD_HAS_QSPI_BUS 1", header)
        self.assertIn("#define ECOS_BOARD_HAS_DISPLAY 1", header)
        self.assertIn("#define ECOS_BOARD_HAS_GPIO_DEMO 0", header)

    def test_selects_panel_specific_pins_and_fixed_wiring(self):
        board = {
            "resources": {
                "qspi-bus": {"controller": 0, "clock_divider": 3},
                "display": {
                    "chip_select": 0,
                    "dc_gpio": {"controller": 0, "pin": 2},
                    "reset_gpio": None,
                    "backlight_gpio": None,
                    "width": 128,
                    "height": 128,
                    "rotation": 0,
                    "horizontal_offset": 2,
                    "vertical_offset": 3,
                },
                "display-st7789": {
                    "chip_select": 0,
                    "dc_gpio": {"controller": 0, "pin": 0},
                    "reset_gpio": {"controller": 0, "pin": 1},
                    "backlight_gpio": None,
                    "width": 240,
                    "height": 240,
                    "rotation": 2,
                    "horizontal_offset": 0,
                    "vertical_offset": 0,
                },
            }
        }
        st7735 = _board_resources_header(board, {"display"})
        self.assertIn("#define ECOS_BOARD_DISPLAY_DC_PIN 2u", st7735)
        self.assertIn(
            "#define ECOS_BOARD_DISPLAY_RESET_PORT ECOS_GPIO_PORT_COUNT",
            st7735,
        )
        self.assertIn("#define ECOS_BOARD_DISPLAY_WIDTH 128u", st7735)

        st7789 = _board_resources_header(board, {"display-st7789"})
        self.assertIn("#define ECOS_BOARD_HAS_QSPI_BUS 1", st7789)
        self.assertIn("#define ECOS_BOARD_DISPLAY_DC_PIN 0u", st7789)
        self.assertIn("#define ECOS_BOARD_DISPLAY_RESET_PIN 1u", st7789)
        self.assertIn("#define ECOS_BOARD_DISPLAY_WIDTH 240u", st7789)
        self.assertIn(
            "#define ECOS_BOARD_DISPLAY_BACKLIGHT_PORT ECOS_GPIO_PORT_COUNT",
            st7789,
        )

    def test_st7789_resource_requires_its_reset_pin(self):
        with self.assertRaisesRegex(ManifestValidationError, "reset_gpio.controller"):
            _display_resource(
                {
                    "driver": "device-st7789",
                    "bus": "qspi-bus",
                    "chip_select": 0,
                    "dc_gpio": {"controller": 0, "pin": 0},
                    "width": 240,
                    "height": 240,
                    "rotation": 2,
                },
                Path("boards/ecos-board.yml"),
                "display-st7789",
            )


if __name__ == "__main__":
    unittest.main()
