// bmc_fsm.v — Конечный автомат для полуформальной верификации (BMC)
// Свойства заданы через assert() и cover() в yosys-формальном стиле.

module bmc_fsm(input clk, rst_n, input [1:0] in, input start,
               output reg [2:0] state, output reg done);

    localparam IDLE=0, RUN=1, WAIT=2, DONE=3, ERROR=4;

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin state <= IDLE; done <= 0; end
        else begin
            case (state)
                IDLE: begin done <= 0; if (start) state <= RUN; end
                RUN: begin
                    if (in == 0) state <= WAIT;
                    else if (in == 3) state <= ERROR;
                    else if (in == 2) state <= DONE;
                end
                WAIT: if (in != 0) state <= RUN;
                DONE: begin done <= 1; if (!start) state <= IDLE; end
                ERROR: if (start) state <= IDLE;
                default: state <= IDLE;
            endcase
        end
    end

    // Формальные свойства (yosys преобразует в $assert/$cover)
    always @(posedge clk) begin
        assert(state <= ERROR);           // state в допустимом диапазоне
        if (done) assert(state == DONE);  // done только в состоянии DONE
    end

endmodule
