//
// The SPI gateway to host controller
//

module SPIGate(
    // Host interface
    input  SCLK,
    input  MOSI,
    output MISO,
    input  nCS,
    // Internal bus
    output [7:0]  RXD,  // Data received from the host
    input  [7:0]  TXD,  // Data to be transmitted to the host
    output [7:0]  ADDR, // Port address received from the host
    output        SEL,  // Port selected. Become active when the the address is valid. Reset after host deselect interface on transfer completion.
    output        TXE,  // Transmit enable. Transmitting port should put data to TXD, it will be latched by gate on second clock edge.
    output        RXE,  // Receive enable. Receiving port should latch RXD on first clock rising edge.
    // Global clock
    input         CLK
    );

parameter CS_FLT_TAPS = 3;

reg [CS_FLT_TAPS-1:0] cs_flt;
reg cs_in;

reg sclk_in;
reg data_in;
reg last_sclk;

always @(posedge CLK)
begin
    cs_flt <= {cs_flt[CS_FLT_TAPS-2:0], ~nCS};
    if (cs_flt == 'b1)
        cs_in <= 1;
    if (cs_flt == 'b0)
        cs_in <= 0;
    sclk_in <= SCLK;
    data_in <= MOSI;
    last_sclk <= sclk_in;
end

wire sclk_edge = sclk_in && sclk_in != last_sclk;

reg[7:0] address;
reg[3:0] address_bits;
wire     address_valid = address_bits[3];

always @(posedge CLK)
begin
    if (!cs_in)
        address_bits <= 0;
    else if (!address_valid && sclk_edge) begin
        address <= {address[6:0], data_in};
        address_bits <= address_bits + 1;
    end;
end

assign ADDR = address;

reg[7:0]  data;
reg[3:0]  data_bits;
wire data_valid = data_bits[3];
reg  need_data;
reg  load_data;
reg  selected;

always @(posedge CLK)
begin
    if (!cs_in) begin
        data_bits <= 0;
        need_data <= 0;
        load_data <= 0;
        selected  <= 0;
    end else if (address_valid) begin
        if (sclk_edge) begin
            data <= {data[6:0], data_in};
            data_bits <= data_bits + 1;
        end;
        selected <= 1;
        if (data_valid)
            data_bits <= 0;
        if (!selected || data_valid)
            need_data <= 1;
        if (need_data)
            load_data <= 1;
        if (load_data) begin 
            data <= TXD;
            need_data <= 0;
            load_data <= 0;
        end
    end
end

assign MISO = data[7];
assign SEL  = selected;
assign RXD  = data;
assign RXE  = data_valid;
assign TXE  = need_data;

endmodule

module IOPort8
    #(
        // If set to 1 the output will only be active for single clock period. After that it will be set to all 0s.
        parameter  ONE_SHOT = 0
    )
    (
    // Address
    input  [7:0]  ADDRESS, // Port address
    input  [7:0]  DI,      // Data input
    output [7:0]  DO,      // Data output
    output        STRB,    // Data strobe - becomes active for one clock period whenever DO is updated
    output        STRT,    // Frame start strobe - becomes active for one clock period whenever port is selected
    output        DONE,    // Frame done strobe - becomes active for one clock period whenever port is deselected

    // Internal bus
    input  [7:0]  RXD,  // Data received from the host
    output [7:0]  TXD,  // Data to be transmitted to the host
    input  [7:0]  ADDR, // Port address received from the host
    input         SEL,  // Port selected. Become active when the the address is valid. Reset after host deselect interface on transfer completion.
    input         TXE,  // Transmit enable. Transmitting port should put data to TXD, it will be latched by gate on second clock edge.
    input         RXE,  // Receive enable. Receiving port should latch RXD on first clock rising edge.
    // Global clock
    input         CLK
    );

reg [7:0] data_rx;
assign DO = data_rx;
assign TXD = (TXE && ADDR == ADDRESS) ? DI : 8'bz;

reg strobe;
assign STRB = strobe;

reg selected;
assign STRT = SEL & ~selected;
assign DONE = ~SEL && selected;

always @(posedge CLK)
begin
    selected <= SEL;
    strobe <= 0;
    if (ONE_SHOT)
        data_rx <= 'b0;
    if (RXE && ADDR == ADDRESS) begin
        data_rx <= RXD;
        strobe <= 1;
    end
end

endmodule

module IOPort16
    #(
        // If set to 1 the output will only be active for single clock period. After that it will be set to all 0s.
        parameter  ONE_SHOT = 0
    )
    (
    // Address
    input  [7:0]  ADDRESS, // Port address
    input  [15:0] DI,      // Data input
    output [15:0] DO,      // Data output
    output        STRB,    // Strobe - becomes active for one clock period whenever DO is updated

    // Internal bus
    input  [7:0]  RXD,  // Data received from the host
    output [7:0]  TXD,  // Data to be transmitted to the host
    input  [7:0]  ADDR, // Port address received from the host
    input         SEL,  // Port selected. Become active when the the address is valid. Reset after host deselect interface on transfer completion.
    input         TXE,  // Transmit enable. Transmitting port should put data to TXD, it will be latched by gate on second clock edge.
    input         RXE,  // Receive enable. Receiving port should latch RXD on first clock rising edge.
    // Global clock
    input         CLK
    );

reg [15:0] data_rx;
reg [15:0] data_out;
assign DO = data_out;

reg strobe;
assign STRB = strobe;

reg hbyte;
assign TXD = (TXE && ADDR == ADDRESS) ? (!hbyte ? DI[7:0] : DI[15:8]) : 8'bz;

always @(posedge CLK)
begin
    strobe <= 0;
    if (ONE_SHOT)
        data_out <= 'b0;
    if (!SEL && hbyte) begin
        hbyte <= 0;
        data_out <= data_rx;
    end
    if (RXE && ADDR == ADDRESS) begin
        if (!hbyte)
            data_rx[7:0] <= RXD;
        else
            data_rx[15:8] <= RXD;
        strobe <= 1;
        hbyte <= 1;
    end
end

endmodule
