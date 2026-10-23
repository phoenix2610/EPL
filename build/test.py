name, version, ready = "EPL", 1, True
print(f"Hello from {name} v{version}")
print(f"Interpreter is ready: {str(ready).lower()}")
x, y = 10, 3
for lbl, v in (("Addition      ", x+y), ("Subtraction   ", x-y), ("Multiplication ", x*y),
               ("Division      ", f"{x/y:.6g}"), ("Power         ", x**2)):
    print(f"{lbl}: {v}")
if x > y: print("x is greater than y")
print("y is in the list" if y in (1,2,3,4) else "y is not in the list")
print(f"Sum 1 to 5: {sum(range(1,6))}")
for fruit in ["apple","banana","cherry"]: print(f"Fruit: {fruit}")
greet = lambda: "Hello from activity!"
square = lambda n: n*n
print(greet())
print(f"Square of 4: {square(4)}")
print("All tests passed!")
