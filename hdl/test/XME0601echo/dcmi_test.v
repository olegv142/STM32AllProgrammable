`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    08:21:34 04/15/2020 
// Design Name: 
// Module Name:    DCMITest 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////

//
// The DCMI tester produces packet with 2**LEN_BITS length
// filled with counter bytes starting from zero
//

module DCMITester
#(
    parameter LEN_BITS = 2
)
(
    // Frame start trigger
    input        START,
    // Multiplexed data interface
    output [7:0] MDATA,
    input        DCLKEN,
    // Multiplexed bus request / acknowledge
    output       DREQ,
    input        DACK,
    // Global clock
    input CLK
    );

reg req = 0;
reg [LEN_BITS-1:0] cnt;

assign DREQ = req;
assign MDATA = DACK ? cnt : 8'bz;

always @(posedge CLK)
begin
    if (START) begin
        req <= 1;
        cnt <= 0;
    end
    if (DACK && DCLKEN) begin
        cnt <= cnt + 1;
        if (&cnt)
            // last byte transmitted
            req <= 0;
    end
end

endmodule

module DCMITxBuff 
#(
    parameter LEN_BITS = 10
)
(
    // Data input API
    input [7:0]  DI,  // Data input
    input        WR,  // Write enable
    input        RST, // Write pointer reset

    // Frame start trigger
    input        START,

    // Multiplexed data interface
    output [7:0] MDATA,
    input        DCLKEN,
    // Multiplexed bus request / acknowledge
    output       DREQ,
    input        DACK,

    // Global clock
    input CLK
    );

localparam MAX_LEN = 1 << LEN_BITS;
reg [7:0] data_ram[0:MAX_LEN-1];
reg  [LEN_BITS-1:0] data_addr;
reg  [LEN_BITS-1:0] data_len;
wire [LEN_BITS-1:0] next_addr = data_addr + 1;

reg req = 0;
assign DREQ = req;
assign MDATA = DACK ? data_ram[data_addr] : 8'bz;

always @(posedge CLK)
begin
    if (RST)
        data_addr <= 0;
    if (WR) begin
        data_ram[data_addr] <= DI;
        data_addr <= data_addr + 1;
    end
    if (START) begin
        req <= 1;
        data_len <= data_addr;
        data_addr <= 0;
    end
    if (DACK && DCLKEN) begin
        data_addr <= next_addr;
        if (next_addr == data_len)
            req <= 0;
    end
end

endmodule
