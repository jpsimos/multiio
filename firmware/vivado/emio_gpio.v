`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Author: Jacob A. Psimos 
// 
// Create Date: 03/13/2026 08:57:21 PM
// Design Name: multiio_design_wrapper
// Module Name: emio_gpio
// Project Name: MultiIO
//////////////////////////////////////////////////////////////////////////////////

module emio_gpio #(
    parameter EMIO_WIDTH = 6
)(
    // EMIO Input
    (* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 EMIO_GPIO_RTL TRI_T" *)
    input [EMIO_WIDTH-1:0] gpio_tri_t,
    (* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 EMIO_GPIO_RTL TRI_O" *)
    input [EMIO_WIDTH-1:0] gpio_tri_o,
    (* X_INTERFACE_INFO = "xilinx.com:interface:gpio:1.0 EMIO_GPIO_RTL TRI_I" *)
    output [EMIO_WIDTH-1:0] gpio_tri_i,
  
    // ----------------------------------------------------------------------------------
    
    input pl_button0,
    input pl_button1,
    output pl_led0,
    output pl_led1,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 GEM0_PHY_RSTN RST" *)
    (* X_INTERFACE_PARAMETER = "POLARITY ACTIVE_LOW" *)
    output gem0_phy_rn,
    (* X_INTERFACE_INFO = "xilinx.com:signal:interrupt:1.0 RTC_INTN INTERRUPT" *)
    (* X_INTERFACE_PARAMETER = "SENSITIVITY LEVEL_LOW" *)
    input rtc_intr
);

localparam integer PL_BUTTON0_OFFSET = 0;
localparam integer PL_BUTTON1_OFFSET = 1;
localparam integer PL_LED0_OFFSET = 2;
localparam integer PL_LED1_OFFSET = 3;
localparam integer GEM0_PHY_RSTN_OFFSET = 4;
localparam integer RTC_INTR_OFFSET = 5;

// PS drives T = 0 when OUTPUT
wire [EMIO_WIDTH-1:0] gpio_i;

genvar e;
generate
    for(e = 0; e < EMIO_WIDTH; e = e + 1) begin
        assign gpio_tri_i[e] = gpio_i[e];
        
        if(e == PL_BUTTON0_OFFSET)
            assign gpio_i[PL_BUTTON0_OFFSET] = pl_button0;
            
        else if(e == PL_BUTTON1_OFFSET)
            assign gpio_i[PL_BUTTON1_OFFSET] = pl_button1;
            
        else if(e == PL_LED0_OFFSET) begin
            assign gpio_i[PL_LED0_OFFSET] = gpio_tri_t[PL_LED0_OFFSET] ? 1'b1 : gpio_tri_o[PL_LED0_OFFSET];
            assign pl_led0 = gpio_i[PL_LED0_OFFSET];
            
        end else if(e == PL_LED1_OFFSET) begin
            assign gpio_i[PL_LED1_OFFSET] = gpio_tri_t[PL_LED1_OFFSET] ? 1'b1 : gpio_tri_o[PL_LED1_OFFSET];
            assign pl_led1 = gpio_i[PL_LED1_OFFSET];
            
        end else if(e == GEM0_PHY_RSTN_OFFSET) begin
            assign gpio_i[GEM0_PHY_RSTN_OFFSET] = gpio_tri_t[GEM0_PHY_RSTN_OFFSET] ? 1'b1 : gpio_tri_o[GEM0_PHY_RSTN_OFFSET];
            assign gem0_phy_rn = gpio_i[GEM0_PHY_RSTN_OFFSET];
            
        end else if(e == RTC_INTR_OFFSET) begin
            assign gpio_i[RTC_INTR_OFFSET] = rtc_intr;
            
        end else
            assign gpio_i[e] = 1'b0;
    end
endgenerate

endmodule
