# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

VAR
    Counter : DINT;
    Result  : DINT;
    Flag    : BOOL;
END_VAR

PROGRAM Main
    Counter := 0;
    Result := 0;

    IF Counter = 0 THEN
        Result := 1;
    ELSIF Counter = 1 THEN
        Result := 2;
    ELSE
        Result := 3;
    END_IF;

    WHILE Counter < 3 DO
        Counter := Counter + 1;
    END_WHILE;

    REPEAT
        Result := Result + Counter;
        Counter := Counter - 1;
    UNTIL Counter <= 0;
    END_REPEAT;

    FOR Counter := 1 TO 4 BY 1 DO
        Result := Result MOD 5;
    END_FOR;

    Flag := TRUE XOR FALSE;
    Result := -Result;
END_PROGRAM
