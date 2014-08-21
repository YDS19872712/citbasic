# CIT-BASIC

```basic
10 N% = RND 100
20 INPUT "Guess the number in range from 0 to 100?", G%
25 T% = T% + 1
30 IF G% = N% GOTO 70
40 IF G% > N% THEN PRINT "My number is less!"
50 IF G% < N% THEN PRINT "My number is greater!"
60 GOTO 20
70 PRINT "Wow, you did it!."
80 END
```

```basic
' Quadratic equation solver:
? "a*x^2 + b*x + c = 0"
Input "a b c >", a, b, c
Let d = b^2 - 4 * a * c

If d < 0 Then &
  ? "There are no real roots!" &
Else &
  If d = 0 GoSub SingleSolution Else GoSub TwoSolutions
End

SingleSolution:
? "x =";-b / (2 * a)
Return

TwoSolutions:
? "x1 =";(-b + sqr(d))/(2 * a)
? "x2 =";(-b - sqr(d))/(2 * a) 
Return
```
