public class Fibonacci {
    public static int fib(int n) {
        if (n <= 1) return n;
        else return fib(n - 1) + fib(n - 2);
    }

    public static void main(String[] args) {
        if(args.length == 0)
            System.out.println("Please specify a number.");
        else {
            int x = Integer.parseInt(args[0]);
            System.out.println(fib(x));
        }
    }
}
