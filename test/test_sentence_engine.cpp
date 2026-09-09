#include "i18n.h"
#include "sentenceEngine.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <set>

void test_fallback_when_no_file()
{
    SentenceEngine engine;
    engine.initMockEnvironment( "test/scratch_no_file.json" );
    assert( !engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 25 );

    std::string sentence;
    bool ok = engine.getNextSentence( sentence );
    assert( ok );
    assert( !sentence.empty() );
    std::cout << "PASS: test_fallback_when_no_file\n";
}

void test_custom_file_indexing()
{
    const char *testFile = "test/scratch_custom.json"; {
        std::ofstream out( testFile );
        out << "[\"Custom joke 1\", \"Custom joke 2 - with escaped \\\"quotes\\\"\", \"Custom joke 3\"]";
    }

    SentenceEngine engine;
    engine.initMockEnvironment( testFile );
    assert( engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 3 );

    std::set<std::string> seen;
    for( size_t i = 0; i < 3; ++i ) {
        std::string s;
        bool ok = engine.getNextSentence( s );
        assert( ok );
        seen.insert( s );
    }
    assert( seen.size() == 3 );
    assert( seen.count( "Custom joke 1" ) == 1 );
    assert( seen.count( "Custom joke 2 - with escaped \"quotes\"" ) == 1 );
    assert( seen.count( "Custom joke 3" ) == 1 );
    std::cout << "PASS: test_custom_file_indexing\n";
}

void test_fisher_yates_non_repeating_cycle()
{
    const char *testFile = "test/scratch_deck.json"; {
        std::ofstream out( testFile );
        out << "[";
        for( int i = 0; i < 10; ++i ) {
            if( i > 0 ) {
                out << ", ";
            }
            out << "\"Joke " << i << "\"";
        }
        out << "]";
    }

    SentenceEngine engine;
    engine.initMockEnvironment( testFile );
    assert( engine.getSentenceCount() == 10 );

    // Verify 10 distinct sentences in the first cycle
    std::set<std::string> cycle1;
    for( int i = 0; i < 10; ++i ) {
        std::string s;
        engine.getNextSentence( s );
        cycle1.insert( s );
    }
    assert( cycle1.size() == 10 );

    // Verify 10 distinct sentences in the second cycle
    std::set<std::string> cycle2;
    for( int i = 0; i < 10; ++i ) {
        std::string s;
        engine.getNextSentence( s );
        cycle2.insert( s );
    }
    assert( cycle2.size() == 10 );
    std::cout << "PASS: test_fisher_yates_non_repeating_cycle\n";
}

void test_quota_rejection()
{
    SentenceEngine engine;
    std::string hugeString( 112641, 'a' ); // 110 KB + 1 byte
    bool ok = engine.validateQuota( hugeString.size() );
    assert( !ok );
    assert( engine.validateQuota( 112640 ) ); // Exactly 110 KB is OK
    std::cout << "PASS: test_quota_rejection\n";
}

void test_atomic_save_and_clear()
{
    const char *testFile = "test/scratch_atomic.json";
    SentenceEngine engine;
    engine.initMockEnvironment( testFile );

    // Initially no file exists
    assert( !engine.hasCustomSentences() );

    // Save valid JSON
    const char *validJson = "[\"Atomic joke 1\", \"Atomic joke 2\", \"Atomic joke 3\"]";
    bool saved = engine.saveCustomJsonAtomic( validJson, std::char_traits<char>::length( validJson ) );
    assert( saved );
    assert( engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 3 );

    std::string s;
    assert( engine.getNextSentence( s ) );
    assert( !s.empty() );

    // Save corrupt JSON should fail and keep old file intact
    const char *corruptJson = "not a valid json array";
    bool corruptSaved = engine.saveCustomJsonAtomic( corruptJson, std::char_traits<char>::length( corruptJson ) );
    assert( !corruptSaved );
    assert( engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 3 );

    // Clear custom sentences
    engine.clearCustomSentences();
    assert( !engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 25 );

    std::remove( testFile );
    std::cout << "PASS: test_atomic_save_and_clear\n";
}

void test_corrupt_file_handling()
{
    const char *testFile = "test/scratch_corrupt.json"; {
        std::ofstream out( testFile );
        out << "[\"Unclosed string...";
    }

    SentenceEngine engine;
    engine.initMockEnvironment( testFile );
    assert( !engine.hasCustomSentences() );
    assert( engine.getSentenceCount() == 25 );

    std::string s;
    assert( engine.getNextSentence( s ) );
    assert( !s.empty() );

    std::remove( testFile );
    std::cout << "PASS: test_corrupt_file_handling\n";
}

int main()
{
    std::cout << "Running SentenceEngine Native Tests...\n";
    I18n::instance().init( Language::English );
    test_fallback_when_no_file();
    test_custom_file_indexing();
    test_fisher_yates_non_repeating_cycle();
    test_quota_rejection();
    test_atomic_save_and_clear();
    test_corrupt_file_handling();

    std::remove( "test/scratch_no_file.json" );
    std::remove( "test/scratch_custom.json" );
    std::remove( "test/scratch_deck.json" );
    std::remove( "test/scratch_atomic.json" );
    std::remove( "test/scratch_corrupt.json" );

    std::cout << "✅ ALL SENTENCE ENGINE TESTS PASSED!\n";
    return 0;
}
