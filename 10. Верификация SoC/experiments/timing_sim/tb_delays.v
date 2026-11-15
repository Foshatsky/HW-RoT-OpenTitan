`timescale 1ns/1ps

module tb_delays;
    reg [3:0] a, b;
    wire [4:0] sum;

    delayed_adder dut(.*);

    initial begin
        $dumpfile("test_timing.vcd");
        $dumpvars(0, tb_delays);

        $display("Time=%0t  a=0  b=0  sum=%d", $time, sum);
        a = 4'd0; b = 4'd0;
        #6;
        $display("Time=%0t  a=0  b=0  sum=%d (expected 0)", $time, sum);

        a = 4'd5; b = 4'd3;
        #6;
        $display("Time=%0t  a=5  b=3  sum=%d (expected 8)", $time, sum);

        a = 4'd9; b = 4'd7;
        #6;
        $display("Time=%0t  a=9  b=7  sum=%d (expected 16)", $time, sum);

        a = 4'd2; b = 4'd4;
        #1;
        $display("Time=%0t  a=2  b=4  sum=%d (expected 6 before delay)", $time, sum);
        #5;
        $display("Time=%0t  a=2  b=4  sum=%d (expected 6 after delay)", $time, sum);

        $finish;
    end
endmodule
