#include <catch2/catch_test_macros.hpp>
#include "ytdlp/utils/js_interpreter.hpp"

using namespace ytdlp::utils;

TEST_CASE("JSInterpreter basic operations", "[utils][js]") {
    SECTION("execute simple expressions") {
        JSInterpreter js;

        REQUIRE(js.execute("1 + 1") == "2");
        REQUIRE(js.execute("'hello' + 'world'") == "helloworld");
        REQUIRE(js.execute("'test'.toUpperCase()") == "TEST");
    }

    SECTION("execute function definitions and calls") {
        JSInterpreter js;

        std::string result = js.execute(
            "function double(x) { return x * 2; } double(21);"
        );
        REQUIRE(result == "42");
    }

    SECTION("evaluate and call_function") {
        JSInterpreter js;

        // Define a function
        js.evaluate("function greet(name) { return 'Hello ' + name; }");

        // Call it
        REQUIRE(js.call_function("greet", "World") == "Hello World");
        REQUIRE(js.call_function("greet", "Alice") == "Hello Alice");
    }

    SECTION("complex function") {
        JSInterpreter js;

        // Simulate a simple n-parameter transformation
        js.evaluate(R"js(
            function transform(input) {
                // Reverse the string
                return input.split('').reverse().join('');
            }
        )js");

        REQUIRE(js.call_function("transform", "hello") == "olleh");
        REQUIRE(js.call_function("transform", "12345") == "54321");
    }
}

TEST_CASE("JSInterpreter string operations", "[utils][js]") {
    JSInterpreter js;

    SECTION("string concatenation") {
        REQUIRE(js.execute("'a' + 'b' + 'c'") == "abc");
    }

    SECTION("string methods") {
        REQUIRE(js.execute("'hello'.substring(1, 4)") == "ell");
        REQUIRE(js.execute("'hello'.charAt(0)") == "h");
        REQUIRE(js.execute("'hello'.indexOf('l')") == "2");
    }

    SECTION("string splitting and joining") {
        REQUIRE(js.execute("'a-b-c'.split('-').join(',')") == "a,b,c");
    }
}

TEST_CASE("JSInterpreter array operations", "[utils][js]") {
    JSInterpreter js;

    SECTION("array methods") {
        REQUIRE(js.execute("[1,2,3].join('-')") == "1-2-3");
        REQUIRE(js.execute("[1,2,3].slice(1,2)[0]") == "2");
    }

    SECTION("array with functions") {
        js.evaluate(R"js(
            function getFirst(arr) {
                return arr[0];
            }
        )js");

        // Note: We only support string arguments, so this tests string indexing
        REQUIRE(js.call_function("getFirst", "hello") == "h");
    }
}

TEST_CASE("JSInterpreter math operations", "[utils][js]") {
    JSInterpreter js;

    SECTION("arithmetic") {
        REQUIRE(js.execute("10 + 5") == "15");
        REQUIRE(js.execute("10 - 5") == "5");
        REQUIRE(js.execute("10 * 5") == "50");
        REQUIRE(js.execute("10 / 5") == "2");
    }

    SECTION("bitwise operations") {
        REQUIRE(js.execute("5 ^ 3") == "6");  // XOR
        REQUIRE(js.execute("8 >> 2") == "2");  // Right shift
        REQUIRE(js.execute("2 << 2") == "8");  // Left shift
    }
}

TEST_CASE("JSInterpreter YouTube-like transformations", "[utils][js]") {
    JSInterpreter js;

    SECTION("simulate simple n-parameter function") {
        // This simulates a simplified version of YouTube's n-parameter function
        js.evaluate(R"js(
            function decrypt_nsig(s) {
                // Simple transformation: reverse and add suffix
                return s.split('').reverse().join('') + '_decrypted';
            }
        )js");

        std::string encrypted = "abc123";
        std::string decrypted = js.call_function("decrypt_nsig", encrypted);
        REQUIRE(decrypted == "321cba_decrypted");
    }

    SECTION("simulate character manipulation") {
        js.evaluate(R"js(
            function transform(input) {
                var result = '';
                for (var i = 0; i < input.length; i++) {
                    var code = input.charCodeAt(i);
                    result += String.fromCharCode(code + 1);
                }
                return result;
            }
        )js");

        REQUIRE(js.call_function("transform", "abc") == "bcd");
    }
}

TEST_CASE("JSInterpreter error handling", "[utils][js]") {
    JSInterpreter js;

    SECTION("syntax error") {
        REQUIRE_THROWS_AS(js.execute("invalid javascript {{{"), JSExecutionError);
    }

    SECTION("runtime error") {
        REQUIRE_THROWS_AS(js.execute("throw new Error('test error')"), JSExecutionError);
    }

    SECTION("undefined function") {
        REQUIRE_THROWS_AS(js.call_function("nonexistent", "arg"), JSExecutionError);
    }

    SECTION("calling non-function") {
        js.evaluate("var notAFunction = 42;");
        REQUIRE_THROWS_AS(js.call_function("notAFunction", "arg"), JSExecutionError);
    }
}

TEST_CASE("JSInterpreter complex scenario", "[utils][js]") {
    JSInterpreter js;

    SECTION("multi-step transformation") {
        // Define helper functions
        js.evaluate(R"js(
            function swap(s, i, j) {
                var arr = s.split('');
                var temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
                return arr.join('');
            }

            function transform(input) {
                var result = input;
                result = swap(result, 0, result.length - 1);
                result = swap(result, 1, result.length - 2);
                return result;
            }
        )js");

        // Transform: swap(0,6) then swap(1,5)
        // "abcdefg" -> "gbcdefa" -> "gfcdeba"
        REQUIRE(js.call_function("transform", "abcdefg") == "gfcdeba");
    }
}
