package com.nomoid.jlox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;

public class Lox {

    private static final Interpreter interpreter = new Interpreter();

    static boolean hadError = false;
    static boolean hadRuntimeError = false;
    static boolean suppressErrors = false;

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64);
        }
        else if (args.length == 1) {
            runFile(args[0]);
        }
        else {
            runPrompt();
        }
    }

    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()), false);

        if (hadError) {
            System.exit(65);
        }
        if (hadRuntimeError) {
            System.exit(70);
        }
    }

    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);

        while (true) {
            System.out.print("> ");
            run(reader.readLine(), true);
            hadError = false;
            hadRuntimeError = false;
        }
    }

    private static void run(String source, boolean repl) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();
        Parser parser = new Parser(tokens);
        List<Stmt> statements = null;
        if (repl) {
            suppressErrors = true;
            // Parse expression
            Expr expression = parser.parseExpression();

            if (hadError) {
                hadError = false;
                suppressErrors = false;
                parser = new Parser(tokens);
            }
            else {
                suppressErrors = false;
                statements = Arrays.asList(new Stmt.Print(expression));
            }            
        }
        if (statements == null) {
            // Parse statements
            statements = parser.parse();

            if (hadError) {
                return;
            }
        }
        
        // Run resolver
        Resolver resolver = new Resolver(interpreter);
        resolver.resolve(statements);

        if (hadError) {
            return;
        }

        interpreter.interpret(statements);
    }

    static void error(int line, String message) {
        report(line, "", message);
    }

    static void error(Token token, String message) {
        if (token.type == TokenType.EOF) {
            report(token.line, " at end", message);
        }
        else {
            report(token.line, " at '" + token.lexeme + "'", message);
        }
    }

    static void runtimeError(RuntimeError error) {
        hadRuntimeError = true;
        if (suppressErrors) {
            return;
        }
        System.err.println(error.getMessage() +
            "\n[line " + error.token.line + "]");
    }

    private static void report(int line, String where, String message) {
        hadError = true;
        if (suppressErrors) {
            return;
        }
        System.err.println(
            "[line " + line + "] Error" + where + ": " + message);
    }
}