#include "xparameters.h"

#include "ov5640/OV5640.h"
#include "ov5640/ScuGicInterruptController.h"
#include "ov5640/PS_GPIO.h"
#include "ov5640/AXI_VDMA.h"
#include "ov5640/PS_IIC.h"
#include "axis_switch/SWITCH_CTL.h"				// Modified for Assignment

#include "MIPI_D_PHY_RX.h"
#include "MIPI_CSI_2_RX.h"

#include "xil_io.h"
#include "xil_printf.h"
#include "xuartps.h"

#define IRPT_CTL_DEVID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define GPIO_DEVID			XPAR_PS7_GPIO_0_DEVICE_ID
#define GPIO_IRPT_ID		XPAR_PS7_GPIO_0_INTR
#define CAM_I2C_DEVID		XPAR_PS7_I2C_0_DEVICE_ID
#define CAM_I2C_IRPT_ID		XPAR_PS7_I2C_0_INTR
#define VDMA_DEVID			XPAR_AXIVDMA_0_DEVICE_ID
#define VDMA_MM2S_IRPT_ID	XPAR_FABRIC_AXI_VDMA_0_MM2S_INTROUT_INTR
#define VDMA_S2MM_IRPT_ID	XPAR_FABRIC_AXI_VDMA_0_S2MM_INTROUT_INTR
#define CAM_I2C_SCLK_RATE	100000

// Modified for Assignment
#define SRC_AXIS_SW_DEVID	XPAR_AXIS_SWITCH_0_DEVICE_ID
#define DST_AXIS_SW_DEVID	XPAR_AXIS_SWITCH_1_DEVICE_ID
#define GPIO_SW_DEVID 		XPAR_GPIO_0_DEVICE_ID

#define DDR_BASE_ADDR		XPAR_DDR_MEM_BASEADDR
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x0A000000)

#define GAMMA_BASE_ADDR     XPAR_AXI_GAMMACORRECTION_0_BASEADDR

using namespace digilent;

void pipeline_mode_change(AXI_VDMA<ScuGicInterruptController>& vdma_driver, OV5640& cam, VideoOutput& vid, Resolution res, OV5640_cfg::mode_t mode)
{
	//Bring up input pipeline back-to-front
	{
		vdma_driver.resetWrite();
		MIPI_CSI_2_RX_mWriteReg(XPAR_CAMERA_IN_MIPI_CSI_2_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, (CR_RESET_MASK & ~CR_ENABLE_MASK));
		MIPI_D_PHY_RX_mWriteReg(XPAR_CAMERA_IN_MIPI_D_PHY_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, (CR_RESET_MASK & ~CR_ENABLE_MASK));
		cam.reset();
	}

	{
		vdma_driver.configureWrite(timing[static_cast<int>(res)].h_active, timing[static_cast<int>(res)].v_active);
		Xil_Out32(GAMMA_BASE_ADDR, 3); // Set Gamma correction factor to 1/1.8
		//TODO CSI-2, D-PHY config here
		cam.init();
	}

	{
		vdma_driver.enableWrite();
		MIPI_CSI_2_RX_mWriteReg(XPAR_CAMERA_IN_MIPI_CSI_2_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, CR_ENABLE_MASK);
		MIPI_D_PHY_RX_mWriteReg(XPAR_CAMERA_IN_MIPI_D_PHY_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, CR_ENABLE_MASK);
		cam.set_mode(mode);
		cam.set_awb(OV5640_cfg::awb_t::AWB_ADVANCED);
	}

	//Bring up output pipeline back-to-front
	{
		vid.reset();
		vdma_driver.resetRead();
	}

	{
		vid.configure(res);
		vdma_driver.configureRead(timing[static_cast<int>(res)].h_active, timing[static_cast<int>(res)].v_active);
	}

	{
		vid.enable();
		vdma_driver.enableRead();
	}
}

int main()
{

	ScuGicInterruptController irpt_ctl(IRPT_CTL_DEVID);
	PS_GPIO<ScuGicInterruptController> gpio_driver(GPIO_DEVID, irpt_ctl, GPIO_IRPT_ID);
	PS_IIC<ScuGicInterruptController> iic_driver(CAM_I2C_DEVID, irpt_ctl, CAM_I2C_IRPT_ID, 100000);
        
    SWITCH_GPIO sw_gpio(GPIO_SW_DEVID);
	AXI_SWITCH src_switch(SRC_AXIS_SW_DEVID);
	AXI_SWITCH dst_switch(DST_AXIS_SW_DEVID);

	SWITCH_CTL axis_switch_ctl(src_switch, dst_switch, sw_gpio, XPAR_AXIS_SWITCH_0_NUM_MI, 0);


	OV5640 cam(iic_driver, gpio_driver);
	AXI_VDMA<ScuGicInterruptController> vdma_driver(VDMA_DEVID, MEM_BASE_ADDR, irpt_ctl,
			VDMA_MM2S_IRPT_ID,
			VDMA_S2MM_IRPT_ID);
	VideoOutput vid(XPAR_VIDEO_OUT_VTG_DEVICE_ID, XPAR_VIDEO_OUT_VTG_DEVICE_ID);

	pipeline_mode_change(vdma_driver, cam, vid, Resolution::R1920_1080_60_PP, OV5640_cfg::mode_t::MODE_1080P_1920_1080_30fps);


	xil_printf("Video init done.\r\n");


	// Liquid lens control
	uint8_t read_char0 = 0;
	uint8_t read_char1 = 0;
	uint8_t read_char2 = 0;
	uint8_t read_char4 = 0;
	uint8_t read_char5 = 0;
	uint16_t reg_addr;
	uint8_t reg_value;
	uint32_t color_values;

	while (1) {

		axis_switch_ctl.updateChannel();

		xil_printf("\r\n\r\n\r\nPcam 5C MAIN OPTIONS\r\n");
		xil_printf("\r\nPlease press the key corresponding to the desired option:");
		xil_printf("\r\n  a. Change Resolution");
		xil_printf("\r\n  b. Change Liquid Lens Focus");
		xil_printf("\r\n  d. Change Image Format (Raw or RGB)");
		xil_printf("\r\n  e. Write a Register Inside the Image Sensor");
		xil_printf("\r\n  f. Read a Register Inside the Image Sensor");
		xil_printf("\r\n  g. Change Gamma Correction Factor Value");
		xil_printf("\r\n  h. Change AWB Settings");
		xil_printf("\r\n  i. Select Algorithm Using Switches\r\n\r\n");


		read_char0 = getchar();
		getchar();
		xil_printf("Read: %d\r\n", read_char0);

		switch(read_char0) {

		case 'a':
			xil_printf("\r\n  Please press the key corresponding to the desired resolution:");
			xil_printf("\r\n    1. 1920 x 1080, 15fps");
			xil_printf("\r\n    2. 1920 x 1080, 30fps");
			read_char1 = getchar();
			getchar();
			xil_printf("\r\nRead: %d", read_char1);
			switch(read_char1) {
			case '1':
				pipeline_mode_change(vdma_driver, cam, vid, Resolution::R1920_1080_60_PP, OV5640_cfg::mode_t::MODE_1080P_1920_1080_15fps);
				xil_printf("Resolution change done.\r\n");
				break;
			case '2':
				pipeline_mode_change(vdma_driver, cam, vid, Resolution::R1920_1080_60_PP, OV5640_cfg::mode_t::MODE_1080P_1920_1080_30fps);
				xil_printf("Resolution change done.\r\n");
				break;
			default:
				xil_printf("\r\n  Selection is outside the available options! Please retry...");
			}
			break;

		case 'b':
			xil_printf("\r\n\r\nPlease enter value of liquid lens register, in hex, with small letters: 0x");
			//A, B, C,..., F need to be entered with small letters
			while (read_char1 < 48) {
				read_char1 = getchar();
			}
			while (read_char2 < 48) {
				read_char2 = getchar();
			}
			getchar();
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char1 <= 57) {
				read_char1 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char1 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char2 <= 57) {
				read_char2 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char2 -= 87;
			}
			cam.writeRegLiquid((uint8_t) (16*read_char1 + read_char2));
			xil_printf("\r\nWrote to liquid lens controller: %x", (uint8_t) (16*read_char1 + read_char2));
			break;

		case 'd':
			xil_printf("\r\n  Please press the key corresponding to the desired setting:");
			xil_printf("\r\n    1. Select image format to be RGB, output still Raw");
			xil_printf("\r\n    2. Select image format & output to both be Raw");
			read_char1 = getchar();
			getchar();
			xil_printf("\r\nRead: %d", read_char1);
			switch(read_char1) {
			case '1':
				cam.set_isp_format(OV5640_cfg::isp_format_t::ISP_RGB);
				xil_printf("Settings change done.\r\n");
				break;
			case '2':
				cam.set_isp_format(OV5640_cfg::isp_format_t::ISP_RAW);
				xil_printf("Settings change done.\r\n");
				break;
			default:
				xil_printf("\r\n  Selection is outside the available options! Please retry...");
			}
			break;

		case 'e':
			xil_printf("\r\nPlease enter address of image sensor register, in hex, with small letters: \r\n");
			//A, B, C,..., F need to be entered with small letters
			while (read_char1 < 48) {
				read_char1 = getchar();
			}
			while (read_char2 < 48) {
				read_char2 = getchar();
			}
			while (read_char4 < 48) {
				read_char4 = getchar();
			}
			while (read_char5 < 48) {
				read_char5 = getchar();
			}
			getchar();
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char1 <= 57) {
				read_char1 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char1 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char2 <= 57) {
				read_char2 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char2 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char4 <= 57) {
				read_char4 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char4 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char5 <= 57) {
				read_char5 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char5 -= 87;
			}
			reg_addr = 16*(16*(16*read_char1 + read_char2)+read_char4)+read_char5;
			xil_printf("Desired Register Address: %x\r\n", reg_addr);

			read_char1 = 0;
			read_char2 = 0;
			xil_printf("\r\nPlease enter value of image sensor register, in hex, with small letters: \r\n");
			//A, B, C,..., F need to be entered with small letters
			while (read_char1 < 48) {
				read_char1 = getchar();
			}
			while (read_char2 < 48) {
				read_char2 = getchar();
			}
			getchar();
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char1 <= 57) {
				read_char1 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char1 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char2 <= 57) {
				read_char2 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char2 -= 87;
			}
			reg_value = 16*read_char1 + read_char2;
			xil_printf("Desired Register Value: %x\r\n", reg_value);
			cam.writeReg(reg_addr, reg_value);
			xil_printf("Register write done.\r\n");

			break;

		case 'f':
			xil_printf("Please enter address of image sensor register, in hex, with small letters: \r\n");
			//A, B, C,..., F need to be entered with small letters
			while (read_char1 < 48) {
				read_char1 = getchar();
			}
			while (read_char2 < 48) {
				read_char2 = getchar();
			}
			while (read_char4 < 48) {
				read_char4 = getchar();
			}
			while (read_char5 < 48) {
				read_char5 = getchar();
			}
			getchar();
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char1 <= 57) {
				read_char1 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char1 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char2 <= 57) {
				read_char2 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char2 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char4 <= 57) {
				read_char4 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char4 -= 87;
			}
			// If character is a digit, convert from ASCII code to a digit between 0 and 9
			if (read_char5 <= 57) {
				read_char5 -= 48;
			}
			// If character is a letter, convert ASCII code to a number between 10 and 15
			else {
				read_char5 -= 87;
			}
			reg_addr = 16*(16*(16*read_char1 + read_char2)+read_char4)+read_char5;
			xil_printf("Desired Register Address: %x\r\n", reg_addr);

			cam.readReg(reg_addr, reg_value);
			xil_printf("Value of Desired Register: %x\r\n", reg_value);

			break;

		case 'g':
			xil_printf("  Please press the key corresponding to the desired Gamma factor:\r\n");
			xil_printf("    1. Gamma Factor = 1\r\n");
			xil_printf("    2. Gamma Factor = 1/1.2\r\n");
			xil_printf("    3. Gamma Factor = 1/1.5\r\n");
			xil_printf("    4. Gamma Factor = 1/1.8\r\n");
			xil_printf("    5. Gamma Factor = 1/2.2\r\n");
			read_char1 = getchar();
			getchar();
			xil_printf("Read: %d\r\n", read_char1);
			// Convert from ASCII to numeric
			read_char1 = read_char1 - 48;
			if ((read_char1 > 0) && (read_char1 < 6)) {
				Xil_Out32(GAMMA_BASE_ADDR, read_char1-1);
				xil_printf("Gamma value changed to 1.\r\n");
			}
			else {
				xil_printf("  Selection is outside the available options! Please retry...\r\n");
			}
			break;

		case 'h':
			xil_printf("  Please press the key corresponding to the desired AWB change:\r\n");
			xil_printf("    1. Enable Advanced AWB\r\n");
			xil_printf("    2. Enable Simple AWB\r\n");
			xil_printf("    3. Disable AWB\r\n");
			read_char1 = getchar();
			getchar();
			xil_printf("Read: %d\r\n", read_char1);
			switch(read_char1) {
			case '1':
				cam.set_awb(OV5640_cfg::awb_t::AWB_ADVANCED);
				xil_printf("Enabled Advanced AWB\r\n");
				break;
			case '2':
				cam.set_awb(OV5640_cfg::awb_t::AWB_SIMPLE);
				xil_printf("Enabled Simple AWB\r\n");
				break;
			case '3':
				cam.set_awb(OV5640_cfg::awb_t::AWB_DISABLED);
				xil_printf("Disabled AWB\r\n");
				break;
			default:
				xil_printf("  Selection is outside the available options! Please retry...\r\n");
			}
			break;

			case 'i': {
			    uint32_t last_switch_value = 0xFFFFFFFF; // Initialize with invalid value

			    // Clear terminal screen
			    xil_printf("\033[2J"); // Clear screen
			    xil_printf("\033[H");  // Cursor home

			    xil_printf("\r\n================ SWITCH CONTROL MODE ================\r\n");
			        xil_printf("Slide the board switches to select an algorithm:\r\n");
			        xil_printf("  0000: Original Image (No Processing)\r\n");
			        xil_printf("  0001: Brightness Adjustment\r\n");
			        xil_printf("  0010: Invert RGB Colors\r\n");
			        xil_printf("  0011: RGB to Grayscale\r\n");
			        xil_printf("  0100: Gaussian Blur\r\n");
			        xil_printf("  0101: Scharr Edge Detection\r\n");
			        xil_printf("  0110: Sobel Edge Detection\r\n");
			        xil_printf("\r\nPress [Enter] to confirm selection, [q] to exit this mode.\r\n");
			        xil_printf("=====================================================\r\n");

			        while (1) {
			            axis_switch_ctl.updateChannel();
			            uint32_t switch_value = sw_gpio.readSwitchGpio();

			            if (switch_value != last_switch_value) {
			                xil_printf("\r\nSelected Switch: ");
			                for (int i = 3; i >= 0; --i) {
			                    xil_printf("%d", (switch_value >> i) & 1);
			                }
			                xil_printf(" (%d) => ", switch_value);
			                switch (switch_value) {
			                    case 0:
			                        xil_printf("Original Image");
			                        break;
			                    case 1:
			                        xil_printf("Brightness Adjustment");
			                        break;
			                    case 2:
			                        xil_printf("Invert RGB Colors");
			                        break;
			                    case 3:
			                        xil_printf("RGB to Grayscale");
			                        break;
			                    case 4:
			                        xil_printf("Gaussian Blur");
			                        break;
			                    case 5:
			                        xil_printf("Scharr Edge Detection");
			                        break;
			                    case 6:
			                        xil_printf("Sobel Edge Detection");
			                        break;
			                    default:
			                        xil_printf("Invalid Switch Position");
			                        break;
			                }
			                xil_printf("\r\n");
			                last_switch_value = switch_value;
			            }

			            if (XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
			                char user_key = getchar();
			                if (user_key == 'q' || user_key == 'Q') {
			                    xil_printf("\r\nExiting Switch Control Mode. Returning to Main Menu...\r\n");
			                    break;
			                } else if (user_key == '\r') {
			                    xil_printf("Algorithm confirmed: ");
			                    switch (switch_value) {
			                        case 0:
			                            xil_printf("Original Image\r\n");
			                            break;
			                        case 1:
			                            xil_printf("Brightness Adjustment\r\n");
			                            break;
			                        case 2:
			                            xil_printf("Invert RGB Colors\r\n");
			                            break;
			                        case 3:
			                            xil_printf("RGB to Grayscale\r\n");
			                            break;
			                        case 4:
			                            xil_printf("Gaussian Blur\r\n");
			                            break;
			                        case 5:
			                            xil_printf("Scharr Edge Detection\r\n");
			                            break;
			                        case 6:
			                            xil_printf("Sobel Edge Detection\r\n");
			                            break;
			                        default:
			                            xil_printf("Invalid Selection\r\n");
			                            break;
			                    }
			                    xil_printf("You can continue sliding switches, or press 'q' to quit.\r\n");

			            }
			        }

			        usleep(200000); // Refresh every 200ms
			    }
			    break;
			}


		default:
			xil_printf("  Selection is outside the available options! Please retry...\r\n");
		}

		read_char1 = 0;
		read_char2 = 0;
		read_char4 = 0;
		read_char5 = 0;
	}

	return 0;
}

