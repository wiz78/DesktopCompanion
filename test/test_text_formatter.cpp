#include "textFormatter.h"

#include <cassert>
#include <iostream>

void test_short_single_line()
{
    const std::string text = "Schiena";
    assert( TextFormatter::canFitInLines( text ) );
    auto lines = TextFormatter::wrapText( text );
    assert( lines.size() == 1 );
    assert( lines[ 0 ] == "Schiena" );
    std::cout << "PASS: test_short_single_line\n";
}

void test_two_and_three_lines()
{
    const std::string text = "Schiena: Croccante";
    assert( TextFormatter::canFitInLines( text ) );
    auto lines = TextFormatter::wrapText( text );
    assert( lines.size() == 2 );
    assert( lines[ 0 ] == "Schiena:" );
    assert( lines[ 1 ] == "Croccante" );

    const std::string text2 = "Pazienza 12% Caffe";
    assert( TextFormatter::canFitInLines( text2 ) );
    auto lines2 = TextFormatter::wrapText( text2 );
    assert( lines2.size() == 2 );
    assert( lines2[ 0 ] == "Pazienza" );
    assert( lines2[ 1 ] == "12% Caffe" );

    const std::string text3 = "Pazienza OttoNove Caffe";
    assert( TextFormatter::canFitInLines( text3 ) );
    auto lines3 = TextFormatter::wrapText( text3 );
    assert( lines3.size() == 3 );
    assert( lines3[ 0 ] == "Pazienza" );
    assert( lines3[ 1 ] == "OttoNove" );
    assert( lines3[ 2 ] == "Caffe" );
    std::cout << "PASS: test_two_and_three_lines\n";
}

void test_exceeding_chars_or_lines()
{
    // Exceeds 40 chars -> should not fit
    const std::string longText = "There is still no cure for the common birthday. - John Glenn";
    assert( !TextFormatter::canFitInLines( longText ) );

    // Fits length (<= 40) but overflows 3 lines (4 words of 9 chars each)
    const std::string fourLines = "wordoneaa wordtwobb wordthree wordfourr";
    assert( !TextFormatter::canFitInLines( fourLines ) );
    std::cout << "PASS: test_exceeding_chars_or_lines\n";
}

void test_long_word_split()
{
    const std::string text = "Supercalifragilistic";
    auto lines = TextFormatter::wrapText( text, 10, 3 );
    assert( lines.size() >= 2 );
    assert( lines[ 0 ].length() <= 10 );
    assert( lines[ 0 ] == "Supercali-" );
    std::cout << "PASS: test_long_word_split\n";
}

void test_empty_string()
{
    const std::string empty = "";
    assert( TextFormatter::canFitInLines( empty ) );
    auto lines = TextFormatter::wrapText( empty );
    assert( lines.empty() );

    const std::string spacesOnly = "     ";
    assert( TextFormatter::canFitInLines( spacesOnly ) );
    auto linesSpaces = TextFormatter::wrapText( spacesOnly );
    assert( linesSpaces.empty() );
    std::cout << "PASS: test_empty_string\n";
}

void test_boundary_length_and_limits()
{
    // Exact 40 chars: 4 words of 9 chars + 3 spaces = 39 chars, total 4 lines
    const std::string text39_4lines = "123456789 123456789 123456789 123456789";
    assert( text39_4lines.length() == 39 );
    assert( !TextFormatter::canFitInLines( text39_4lines ) ); // 4 lines > 3

    // Exact 30 chars: 3 words of 10 chars + 2 spaces = 32 chars (3 lines)
    const std::string text30_3lines = "1234567890 1234567890 1234567890";
    assert( text30_3lines.length() == 32 );
    assert( TextFormatter::canFitInLines( text30_3lines ) );
    auto lines30 = TextFormatter::wrapText( text30_3lines );
    assert( lines30.size() == 3 );
    assert( lines30[ 0 ] == "1234567890" );
    assert( lines30[ 1 ] == "1234567890" );
    assert( lines30[ 2 ] == "1234567890" );

    // 41 chars -> exceeds MAX_WRAPPED_CHARS
    const std::string text41 = "12345678901234567890123456789012345678901";
    assert( text41.length() == 41 );
    assert( !TextFormatter::canFitInLines( text41 ) );
    std::cout << "PASS: test_boundary_length_and_limits\n";
}

int main()
{
    std::cout << "Running TextFormatter Native Tests...\n";
    test_short_single_line();
    test_two_and_three_lines();
    test_exceeding_chars_or_lines();
    test_long_word_split();
    test_empty_string();
    test_boundary_length_and_limits();
    std::cout << "✅ ALL TEXT FORMATTER TESTS PASSED!\n";
    return 0;
}
