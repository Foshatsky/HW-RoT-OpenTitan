module tiny_alu (
    input  wire [3:0] a,
    input  wire [3:0] b,
    input  wire [2:0] op,
    output reg  [3:0] result,
    output reg        carry_flag
);
    always @(*) begin
        carry_flag = 1'b0;
        result     = 4'b0;

        case (op)
            3'b000: {carry_flag, result} = a + b;

            3'b001: begin // SUB — intentionally buggy
                result = a - b;
                carry_flag = (a < b);
                // BUG: check result==0 *before* borrow is resolved;
                // when a==b we override the correct borrow=0 with 1
                if (result == 0)
                    carry_flag = 1;
            end

            3'b010: result = a & b;
            3'b011: result = a | b;
            3'b100: result = a ^ b;
            default: result = 4'b0;
        endcase
    end
endmodule
