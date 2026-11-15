module impl_add(input [3:0] a,b, output [4:0] sum);
  wire c1,c2,c3,c4;
  wire [3:0] s;

  full_adder fa0(a[0], b[0], 1'b0, s[0], c1);
  full_adder fa1(a[1], b[1], c1, s[1], c2);
  full_adder fa2(a[2], b[2], c2, s[2], c3);
  full_adder fa3(a[3], b[3], c3, s[3], c4);

  assign sum = {c4, s};
endmodule

module full_adder(input a,b,cin, output s,cout);
  assign s = a ^ b ^ cin;
  assign cout = (a & b) | (a & cin) | (b & cin);
endmodule
