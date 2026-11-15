`timescale 1ns/1ps

module delayed_adder(input [3:0] a, b, output reg [4:0] sum);
    always @(*) begin
        #5 sum = a + b;
    end
endmodule
