// bmc_counter.v — Bounded Model Checking (BMC) пример
// Полуформальная верификация: проверка свойств до глубины k
// (без индукции, в отличие от полной формальной верификации)

module bmc_counter(
    input wire clk,
    input wire rst_n,
    input wire inc,
    output reg [3:0] count
);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            count <= 0;
        else if (inc)
            count <= count + 1;
    end

    // Свойство 1: счётчик никогда не превышает 15 (4-битный)
    // (формально: assert property (count <= 15);)
    // В yosys через cover/assert:
    // cover: может ли count превысить 15 за k шагов?
    // assert: count всегда <= 15 (нуждается в индукции для полного доказательства)

    // Свойство 2: после сброса count == 0
    // Свойство 3: если inc=0, count не меняется

endmodule
