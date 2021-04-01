//
// Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
//
// OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
// You can use this software according to the terms and conditions of the MulanPSL - 2.0.
// You may obtain a copy of MulanPSL - 2.0 at:
//
//   https://opensource.org/licenses/MulanPSL-2.0
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
// FIT FOR A PARTICULAR PURPOSE.
// See the MulanPSL - 2.0 for more details.
//

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
