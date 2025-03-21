## PROTOTEXT

This is a prototype for a forth-like compiled language. It was written in a week.

```
  ,----Stack Management.---Label Jumps--------------,
  | dup  | a b c c     | jmp  | jeq  | jne          |
  | pop  | a b         | jtr  | jfa  | jlt          |
  | ovr  | a b c b     | jgt  | jge  | jle          |
  | swp  | a c b       :---Word Management----------:
  | nip  | a c         | ret  | return from proc    |
  | rot  | b c a       | :    | last token is word  |
  | cut  | b c         | ,    | last token is label |
  :----Change Mode-----: ;    | next token is label |
  | u8   | i8          |      | reference           |
  | u16  | i16         | .    | exec last token     |
  | u32  | i32         :---Example------------------:
  | u64  | i64         | ( these are comments )     |
  :----ALU-------------| start. (jump to start)     |
  | and  | add  | shr  |                            |
  | or   | sub  | shl  | start: "hello world        |
  | xor  | mul  |      | " loop, u64 0xb u8 1 write |
  | not  | div  |      | ;loop jmp ret.             |
  | com  | mod  |      |                            |
  :----stdout----------:----------------------------:
  | write | expects [length fileno] <- top of stack |
  `--------------------'----------------------------'

```
