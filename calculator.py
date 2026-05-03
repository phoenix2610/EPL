import os

class Calculator:
    def __init__(self):
        self.history = []
        self.running = True

    def clear_screen(self):
        os.system('cls' if os.name == 'nt' else 'clear')

    def add(self, x, y): return x + y
    def subtract(self, x, y): return x - y
    def multiply(self, x, y): return x * y
    def divide(self, x, y):     
        if y == 0:
            return "Error: Division by zero"
        return x / y

    def get_number(self, prompt):
        while True:
            try:
                return float(input(prompt))
            except ValueError:
                print("Invalid input. Please enter a number.")

    def show_menu(self):
        print("\n--- Python CLI Calculator ---")
        print("1. Add (+)")
        print("2. Subtract (-)")
        print("3. Multiply (*)")
        print("4. Divide (/)")
        print("5. View History")
        print("6. Exit")

    def run(self):
        while self.running:
            self.show_menu()
            choice = input("\nSelect an option (1-6): ")

            if choice == '6':
                print("Goodbye!")
                self.running = False
                continue
            
            if choice == '5':
                self.clear_screen()
                print("--- Calculation History ---")
                if not self.history:
                    print("No records found.")
                for entry in self.history:
                    print(entry)
                input("\nPress Enter to return...")
                continue

            if choice in ('1', '2', '3', '4'):
                num1 = self.get_number("Enter first number: ")
                num2 = self.get_number("Enter second number: ")

                if choice == '1':
                    res = self.add(num1, num2)
                    op = "+"
                elif choice == '2':
                    res = self.subtract(num1, num2)
                    op = "-"
                elif choice == '3':
                    res = self.multiply(num1, num2)
                    op = "*"
                elif choice == '4':
                    res = self.divide(num1, num2)
                    op = "/"

                result_str = f"{num1} {op} {num2} = {res}"
                print(f"\nResult: {result_str}")
                self.history.append(result_str)
            else:
                print("Invalid selection.")

if __name__ == "__main__":
    calc = Calculator()
    try:
        calc.run()
    except KeyboardInterrupt:
        print("\nProcess exited.")