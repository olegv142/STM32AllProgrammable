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
    output [15:0] RXD,  // Data received from the host
    input  [15:0] TXD,  // Data to be transmitted to the host
    output [7:0]  ADDR, // Port address received from the host
    output        SEL,  // Port selected. Become active when the the address is valid. Reset after host deselect interface on transfer completion.
    output        TXE,  // Transmit enable. Transmitting port should put data to TXD, it will be latched by gate on second clock edge.
    output        RXE,  // Receive enable. Receiving port should latch RXD on first clock rising edge.
    // Global clock
    input         CLK
    );

reg cs_in;
reg sclk_in;
reg last_sclk;

always @(posedge CLK)
begin
    cs_in <= ~nCS;
    sclk_in <= SCLK;
    last_sclk <= sclk_in;
end

wire sclk_edge = sclk_in && sclk_in != last_sclk;

reg[7:0] address;
reg[3:0] address_bits;
wire     address_valid = address_bits[3];

always @(posedge CLK)
begin
    if (!cs_in)
        address_bits <= '0;
    else if (!address_valid && sclk_edge) begin
        address <= {address[6:0], MOSI};
        address_bits <= address_bits + 1;
    end;
end

assign ADDR = address;
assign SEL  = address_valid;

reg[15:0] data;
reg[4:0]  data_bits;
wire last_data_bit = & data_bits[3:0];
wire data_valid = data_bits[4];
reg  need_data;
reg  load_data;

always @(posedge CLK)
begin
    if (!cs_in) begin
        data_bits <= '0;
        need_data <= 1;
    end else if (address_valid) begin
        if (sclk_edge) begin
            data <= {data[14:0], MOSI};
            data_bits <= data_bits + 1;
            if (last_data_bit)
                need_data <= 1;
        end;
        if data_valid
            data_bits <= '0;
        load_data <= need_data;
        if load_data begin 
            data <= TXD;
            need_data <= 0;
        end
    end
end

assign MISO = data[15];
assign RXD = data;
assign RXE = data_valid;
assign TXE = need_data && address_valid;
