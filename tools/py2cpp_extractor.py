#!/usr/bin/env python3
"""
Python-to-C++ Extractor Converter for yt-dlp

Converts Python extractor classes to C++ equivalents.
Automates 70-80% of the conversion process.

Usage:
    python py2cpp_extractor.py <python_extractor_file>

Example:
    python py2cpp_extractor.py /path/to/yt-dlp/yt_dlp/extractor/vimeo.py

Outputs:
    - include/ytdlp/extractor/<name>.hpp
    - src/extractor/<name>.cpp
    - tests/unit/<name>_extractor_test.cpp
"""

import ast
import re
import os
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, field


@dataclass
class ExtractorInfo:
    """Parsed information from Python extractor."""
    class_name: str
    ie_name: str = ""
    ie_key: str = ""
    valid_url: str = ""
    tests: List[Dict] = field(default_factory=list)
    methods: Dict[str, ast.FunctionDef] = field(default_factory=dict)
    imports: List[str] = field(default_factory=list)
    age_limit: int = 0

    def cpp_class_name(self) -> str:
        """Get C++ class name (same as Python but Extractor suffix)."""
        return self.class_name

    def cpp_filename(self) -> str:
        """Get C++ filename (snake_case)."""
        # Convert CamelCase to snake_case
        name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', self.class_name)
        name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()
        # Remove _ie or _extractor suffix if present
        name = name.replace('_ie', '').replace('_extractor', '')
        return name


class ExtractorParser:
    """Parses Python extractor files using AST."""

    def __init__(self, filepath: str):
        self.filepath = filepath
        self.source = Path(filepath).read_text()
        self.tree = ast.parse(self.source)

    def parse(self) -> Optional[ExtractorInfo]:
        """Parse the Python file and extract information."""
        for node in ast.walk(self.tree):
            if isinstance(node, ast.ClassDef):
                # Look for InfoExtractor subclasses
                for base in node.bases:
                    base_name = self._get_name(base)
                    if 'InfoExtractor' in base_name or 'IE' in base_name:
                        return self._parse_extractor_class(node)
        return None

    def _parse_extractor_class(self, node: ast.ClassDef) -> ExtractorInfo:
        """Parse an extractor class node."""
        info = ExtractorInfo(class_name=node.name)

        # Parse class attributes
        for item in node.body:
            if isinstance(item, ast.Assign):
                self._parse_class_attribute(item, info)
            elif isinstance(item, ast.FunctionDef):
                info.methods[item.name] = item

        # Extract IE_NAME and _IE_NAME
        if not info.ie_name:
            info.ie_name = node.name.replace('IE', '').replace('Extractor', '')

        if not info.ie_key:
            info.ie_key = info.ie_name

        return info

    def _parse_class_attribute(self, node: ast.Assign, info: ExtractorInfo):
        """Parse class-level attribute assignments."""
        for target in node.targets:
            if isinstance(target, ast.Name):
                name = target.id

                if name == '_VALID_URL' or name == 'VALID_URL':
                    info.valid_url = self._extract_string(node.value)
                elif name == 'IE_NAME' or name == '_IE_NAME':
                    info.ie_name = self._extract_string(node.value)
                elif name == 'IE_KEY' or name == '_IE_KEY':
                    info.ie_key = self._extract_string(node.value)
                elif name == '_TESTS' or name == 'TESTS':
                    info.tests = self._extract_tests(node.value)
                elif name == '_AGE_LIMIT' or name == 'AGE_LIMIT':
                    if isinstance(node.value, ast.Constant):
                        info.age_limit = node.value.value

    def _extract_string(self, node) -> str:
        """Extract string value from AST node."""
        if isinstance(node, ast.Constant):
            return str(node.value)
        elif isinstance(node, ast.Str):  # Python 3.7 compatibility
            return node.s
        elif isinstance(node, ast.JoinedStr):  # f-string
            # For f-strings, try to extract the pattern
            parts = []
            for value in node.values:
                if isinstance(value, ast.Constant):
                    parts.append(str(value.value))
                else:
                    parts.append("{}")
            return ''.join(parts)
        return ""

    def _extract_tests(self, node) -> List[Dict]:
        """Extract test cases from _TESTS attribute."""
        tests = []
        if isinstance(node, ast.List):
            for item in node.elts:
                if isinstance(item, ast.Dict):
                    test = {}
                    for key, value in zip(item.keys, item.values):
                        if isinstance(key, ast.Constant):
                            key_name = key.value
                            if key_name == 'url':
                                test['url'] = self._extract_string(value)
                            elif key_name == 'only_matching':
                                test['only_matching'] = True
                    tests.append(test)
        return tests

    def _get_name(self, node) -> str:
        """Get name from various AST node types."""
        if isinstance(node, ast.Name):
            return node.id
        elif isinstance(node, ast.Attribute):
            return node.attr
        return ""


class CppHeaderGenerator:
    """Generates C++ header file."""

    def __init__(self, info: ExtractorInfo):
        self.info = info

    def generate(self) -> str:
        """Generate complete C++ header."""
        guard_name = f"YTDLP_EXTRACTOR_{self.info.cpp_filename().upper()}_HPP"

        return f"""#ifndef {guard_name}
#define {guard_name}

#include "info_extractor.hpp"
#include <string>
#include <regex>
#include <vector>

namespace ytdlp::extractor {{

/**
 * {self.info.ie_name} video extractor.
 *
 * Extracted from Python yt-dlp extractor.
 * Some parts may require manual conversion (marked with TODO).
 *
 * Original class: {self.info.class_name}
 */
class {self.info.cpp_class_name()} : public InfoExtractor {{
public:
    /**
     * Construct {self.info.cpp_class_name()}.
     * @param downloader Pointer to YoutubeDL instance
     */
    explicit {self.info.cpp_class_name()}(core::YoutubeDL* downloader = nullptr);

    /**
     * Get extractor key.
     * @return "{self.info.ie_key}"
     */
    std::string ie_key() const override;

    /**
     * Get extractor display name.
     * @return "{self.info.ie_name}"
     */
    std::string ie_name() const override;

    /**
     * Check if URL is valid for this extractor.
     * @param url URL to check
     * @return True if URL matches patterns
     */
    static bool suitable(const std::string& url);

    /**
     * Extract video ID from URL.
     * @param url URL
     * @return Video ID
     * @throws std::runtime_error if video ID cannot be extracted
     */
    static std::string extract_id(const std::string& url);

protected:
    /**
     * Extract video information from URL.
     *
     * @param url URL
     * @return InfoDict with video metadata
     */
    core::InfoDict _real_extract(const std::string& url) override;

private:
    /**
     * URL patterns for this extractor.
     * Generated from Python _VALID_URL.
     */
    static const std::vector<std::regex> URL_PATTERNS;
}};

}} // namespace ytdlp::extractor

#endif // {guard_name}
"""


class CppImplementationGenerator:
    """Generates C++ implementation file."""

    def __init__(self, info: ExtractorInfo):
        self.info = info

    def generate(self) -> str:
        """Generate complete C++ implementation."""
        return f"""#include "ytdlp/extractor/{self.info.cpp_filename()}.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {{

// TODO: Convert Python _VALID_URL regex to C++ std::regex
// Original pattern: {self.info.valid_url}
const std::vector<std::regex> {self.info.cpp_class_name()}::URL_PATTERNS = {{
    std::regex(R"({self._escape_regex(self.info.valid_url)})", std::regex::icase),
}};

{self.info.cpp_class_name()}::{self.info.cpp_class_name()}(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {{
}}

std::string {self.info.cpp_class_name()}::ie_key() const {{
    return "{self.info.ie_key}";
}}

std::string {self.info.cpp_class_name()}::ie_name() const {{
    return "{self.info.ie_name}";
}}

bool {self.info.cpp_class_name()}::suitable(const std::string& url) {{
    for (const auto& pattern : URL_PATTERNS) {{
        if (std::regex_search(url, pattern)) {{
            return true;
        }}
    }}
    return false;
}}

std::string {self.info.cpp_class_name()}::extract_id(const std::string& url) {{
    for (const auto& pattern : URL_PATTERNS) {{
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {{
            if (match.size() > 1) {{
                return match[1].str();
            }}
        }}
    }}
    throw std::runtime_error("Unable to extract video ID from URL: " + url);
}}

core::InfoDict {self.info.cpp_class_name()}::_real_extract(const std::string& url) {{
    // Extract video ID
    std::string video_id = extract_id(url);

    report_extraction(video_id);

    // TODO: Convert Python _real_extract() logic
    // Download webpage
    std::string webpage = _download_webpage(
        url,
        video_id,
        "Downloading video page"
    );

    // TODO: Add extraction logic here
    // - Extract config/JSON data
    // - Extract metadata
    // - Extract formats

    core::InfoDict info;
    info["id"] = video_id;
    info["extractor"] = ie_key();
    info["extractor_key"] = ie_key();
    info["webpage_url"] = url;
    info["_type"] = "video";

    // TODO: Populate info dict with extracted data

    return info;
}}

}} // namespace ytdlp::extractor
"""

    def _escape_regex(self, pattern: str) -> str:
        """Escape regex pattern for C++ raw string."""
        # Basic escaping - may need refinement
        return pattern.replace('\\', '\\\\')


class CppTestGenerator:
    """Generates C++ test file."""

    def __init__(self, info: ExtractorInfo):
        self.info = info

    def generate(self) -> str:
        """Generate C++ test file."""
        test_cases = self._generate_test_cases()

        return f"""#include <catch2/catch_test_macros.hpp>
#include "ytdlp/extractor/{self.info.cpp_filename()}.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/core/info_dict.hpp"

using namespace ytdlp::extractor;
using namespace ytdlp::core;

TEST_CASE("{self.info.cpp_class_name()} construction", "[extractor][{self.info.cpp_filename()}]") {{
    {self.info.cpp_class_name()} extractor;

    REQUIRE(extractor.ie_key() == "{self.info.ie_key}");
    REQUIRE(extractor.ie_name() == "{self.info.ie_name}");
}}

TEST_CASE("{self.info.cpp_class_name()} URL pattern matching", "[extractor][{self.info.cpp_filename()}]") {{
{test_cases}
}}

TEST_CASE("{self.info.cpp_class_name()} video ID extraction", "[extractor][{self.info.cpp_filename()}]") {{
    // TODO: Add video ID extraction tests
    // Use test URLs from Python _TESTS
}}

TEST_CASE("{self.info.cpp_class_name()} with YoutubeDL integration", "[extractor][{self.info.cpp_filename()}]") {{
    SECTION("works with YoutubeDL instance") {{
        YoutubeDLParams params;
        params.quiet = true;
        YoutubeDL ydl(params);

        {self.info.cpp_class_name()} extractor(&ydl);

        REQUIRE(extractor.downloader() == &ydl);
        REQUIRE(extractor.ie_key() == "{self.info.ie_key}");
    }}
}}
"""

    def _generate_test_cases(self) -> str:
        """Generate test cases from Python _TESTS."""
        if not self.info.tests:
            return """    // TODO: Add URL pattern tests
    // No _TESTS found in Python extractor"""

        sections = []
        for i, test in enumerate(self.info.tests[:5]):  # Limit to first 5 tests
            if 'url' in test:
                url = test['url']
                sections.append(f"""    SECTION("matches test URL {i+1}") {{
        REQUIRE({self.info.cpp_class_name()}::suitable("{url}"));
    }}""")

        return '\n\n'.join(sections)


def main():
    """Main converter entry point."""
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    python_file = sys.argv[1]

    if not os.path.exists(python_file):
        print(f"Error: File not found: {python_file}")
        sys.exit(1)

    print(f"🔍 Parsing Python extractor: {python_file}")

    # Parse Python extractor
    parser = ExtractorParser(python_file)
    info = parser.parse()

    if not info:
        print("❌ Error: Could not find InfoExtractor subclass in file")
        sys.exit(1)

    print(f"✓ Found extractor: {info.class_name}")
    print(f"  IE Name: {info.ie_name}")
    print(f"  IE Key: {info.ie_key}")
    print(f"  Valid URL: {info.valid_url[:80]}..." if len(info.valid_url) > 80 else f"  Valid URL: {info.valid_url}")
    print(f"  Tests: {len(info.tests)} test cases found")

    # Generate C++ files
    print(f"\n📝 Generating C++ code...")

    # Project root
    project_root = Path(__file__).parent.parent

    # Generate header
    header_gen = CppHeaderGenerator(info)
    header_content = header_gen.generate()
    header_path = project_root / "include" / "ytdlp" / "extractor" / f"{info.cpp_filename()}.hpp"
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(header_content)
    print(f"✓ Generated: {header_path}")

    # Generate implementation
    impl_gen = CppImplementationGenerator(info)
    impl_content = impl_gen.generate()
    impl_path = project_root / "src" / "extractor" / f"{info.cpp_filename()}.cpp"
    impl_path.parent.mkdir(parents=True, exist_ok=True)
    impl_path.write_text(impl_content)
    print(f"✓ Generated: {impl_path}")

    # Generate tests
    test_gen = CppTestGenerator(info)
    test_content = test_gen.generate()
    test_path = project_root / "tests" / "unit" / f"{info.cpp_filename()}_extractor_test.cpp"
    test_path.parent.mkdir(parents=True, exist_ok=True)
    test_path.write_text(test_content)
    print(f"✓ Generated: {test_path}")

    print(f"\n✅ Conversion complete!")
    print(f"\n⚠️  Manual work needed:")
    print(f"   - Review and fix TODO comments in generated code")
    print(f"   - Convert _real_extract() logic from Python")
    print(f"   - Add helper methods as needed")
    print(f"   - Complete test cases")
    print(f"   - Add to tests/unit/CMakeLists.txt")
    print(f"\n💡 Estimated: ~70% automated, ~30% manual work remaining")


if __name__ == '__main__':
    main()
