package com.nomoid.jloxtool;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
    public static void main(String[] args) throws IOException{
        if (args.length != 1) {
            System.err.println("Usage: generate_ast [output directory]");
            System.exit(1);
        }
        String outputDir = args[0];
        defineAst(outputDir, "Expr", Arrays.asList(
            "Assign   : Token name, Token operator, Expr value",
            "Binary   : Expr left, Token operator, Expr right",
            "Call     : Expr callee, Token paren, List<Expr> arguments",
            "Get      : Expr object, Token name",
            "Grouping : Expr expression",
            "Lambda   : Token keyword, List<Token> params, List<Stmt> body",
            "Literal  : Object value",
            "Logical  : Expr left, Token operator, Expr right",
            "Set      : Expr object, Token name, Token operator, Expr value",
            "This     : Token keyword",
            "Super    : Token keyword, Token method",
            "Unary    : Token operator, Expr right",
            "Ternary  : Expr left, Token operator, Expr center, Expr right",
            "Variable : Token name"
        ));
        defineAst(outputDir, "Stmt", Arrays.asList(
            "Block      : List<Stmt> statements",
            "Class      : Token name, Expr.Variable superclass," +
                " List<Stmt.Function> methods, List<Stmt.Function> statics," +
                " List<Stmt.Getter> getters",
            "Break      : Token token",
            "Expression : Expr expression",
            "Function   : Token name, List<Token> params, List<Stmt> body",
            "Getter     : Token name, List<Stmt> body",
            "If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
            "Print      : Expr expression",
            "Return     : Token keyword, Expr value",
            "Var        : Token name, Expr initializer",
            "While      : Expr condition, Stmt body"
        ));
    }

    private static void defineAst(String outputDir, String baseName,
            List<String> types) throws IOException {
        String path = outputDir + File.separator + baseName + ".java";
        PrintWriter writer = new PrintWriter(path, "UTF-8");

        writer.println("package com.nomoid.jlox;");
        writer.println();
        writer.println("import java.util.List;");
        writer.println();
        writer.println("abstract class " + baseName + " {");

        defineVisitor(writer, baseName, types);

        for (String type : types) {
            String[] parts = type.split(":");
            String className = parts[0].trim();
            String fields = "";
            if (parts.length > 1) {
                fields = parts[1].trim();
            }
            defineType(writer, baseName, className, fields);
        }

        // The base accept() method
        writer.println();
        writer.println(indent(1,
            "abstract <R> R accept(Visitor<R> visitor);"));

        writer.println("}");
        writer.close();
    }

    private static void defineType(PrintWriter writer, String baseName,
            String className, String fieldList) {
        writer.println(indent(1, "static class " + className + " extends " +
            baseName + " {"));
        // Constructor
        writer.println(indent(2, className + "(" + fieldList + ") {"));

        String[] fields = new String[0];
        // Store parameters in fields
        if (fieldList.length() > 0) {
            fields = fieldList.split(", ");
        }

        for (String field : fields) {
            String name = field.split(" ")[1];
            writer.println(indent(3, "this." + name + " = " + name + ";"));
        }

        writer.println(indent(2, "}"));

        // Visitor pattern
        writer.println();
        writer.println(indent(2, "<R> R accept(Visitor<R> visitor) {"));
        writer.println(indent(3, "return visitor.visit" + className +
            baseName + "(this);"));
        writer.println(indent(2, "}"));

        // Fields
        writer.println();
        for (String field : fields) {
            writer.println(indent(2, "final " + field + ";"));
        }

        writer.println(indent(1, "}"));
    }

    private static void defineVisitor(PrintWriter writer, String baseName,
            List<String> types) {
        writer.println(indent(1, "interface Visitor<R> {"));

        for (String type : types) {
            String typeName = type.split(":")[0].trim();
            writer.println(indent(2, "R visit" + typeName + baseName + "(" +
                typeName + " " + baseName.toLowerCase() + ");"));
        }

        writer.println(indent(1, "}"));
    }

    private static final String INDENT = "    ";

    private static String indent(int times, String s) {
        String out = "";
        for (int i = 0; i < times; i++) {
            out += INDENT;
        }
        out += s;
        return out;
    }
}