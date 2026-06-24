(* MOD-DEV-70 test:
 * Blink each LED at 250 ms when its matching DIP is ON.
 * DIP n -> LED n, channels 0..7.
 *)

VAR
    Dip1  AT %IX0.0 : BOOL;
    Dip2  AT %IX0.1 : BOOL;
    Dip3  AT %IX0.2 : BOOL;
    Dip4  AT %IX0.3 : BOOL;
    Dip5  AT %IX0.4 : BOOL;
    Dip6  AT %IX0.5 : BOOL;
    Dip7  AT %IX0.6 : BOOL;
    Dip8  AT %IX0.7 : BOOL;

    Led1  AT %QX0.0 : BOOL;
    Led2  AT %QX0.1 : BOOL;
    Led3  AT %QX0.2 : BOOL;
    Led4  AT %QX0.3 : BOOL;
    Led5  AT %QX0.4 : BOOL;
    Led6  AT %QX0.5 : BOOL;
    Led7  AT %QX0.6 : BOOL;
    Led8  AT %QX0.7 : BOOL;

    BlinkState : BOOL;
    TickCounter : DINT;


END_VAR

PROGRAM Main
    (* 100 Hz NetBurner tick gives 10 ms scans, so 25 scans ~= 250 ms *)
    TickCounter := TickCounter + 1;
    IF TickCounter >= 25 THEN
        TickCounter := 0;
        BlinkState := NOT BlinkState;
    END_IF;

    IF Dip1 THEN Led1 := BlinkState; ELSE Led1 := FALSE; END_IF;
    IF Dip2 THEN Led2 := BlinkState; ELSE Led2 := FALSE; END_IF;
    IF Dip3 THEN Led3 := BlinkState; ELSE Led3 := FALSE; END_IF;
    IF Dip4 THEN Led4 := BlinkState; ELSE Led4 := FALSE; END_IF;
    IF Dip5 THEN Led5 := BlinkState; ELSE Led5 := FALSE; END_IF;
    IF Dip6 THEN Led6 := BlinkState; ELSE Led6 := FALSE; END_IF;
    IF Dip7 THEN Led7 := BlinkState; ELSE Led7 := FALSE; END_IF;
    IF Dip8 THEN Led8 := BlinkState; ELSE Led8 := FALSE; END_IF;
END_PROGRAM
