#include "i18n.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

void test_slap_gag_strings()
{
    auto &i18n = I18n::instance();
    i18n.setLanguage( Language::English );
    assert( std::string( i18n.get( StringId::SlapSignalLost ) ) == "SIGNAL LOST!" );
    assert( std::string( i18n.get( StringId::SlapReject ) ) == "NO! SLAP IT!" );

    i18n.setLanguage( Language::Italian );
    assert( std::string( i18n.get( StringId::SlapSignalLost ) ) == "SEGNALE PERSO!" );
    assert( std::string( i18n.get( StringId::SlapReject ) ) == "NO! DAI UNA BOTTA!" );
    std::cout << "PASS: test_slap_gag_strings\n";
}

void test_default_language()
{
    auto &i18n = I18n::instance();
    i18n.init( Language::English );
    assert( i18n.getLanguage() == Language::English );
    assert( std::strcmp( i18n.getLanguageCode(), "en" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::SlapHeader ), "ERROR 50" ) == 0 );
    std::cout << "PASS: test_default_language\n";
}

void test_language_switching()
{
    auto &i18n = I18n::instance();
    i18n.setLanguage( Language::Italian );
    assert( i18n.getLanguage() == Language::Italian );
    assert( std::strcmp( i18n.getLanguageCode(), "it" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::SlapHeader ), "ERRORE 50" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::TempInternalPrefix ), "INT:" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::HumidityPrefix ), "UMID:" ) == 0 );

    bool ok = i18n.setLanguageCode( "en" );
    assert( ok );
    assert( i18n.getLanguage() == Language::English );
    assert( std::strcmp( i18n.get( StringId::SlapHeader ), "ERROR 50" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::TempInternalPrefix ), "IN:" ) == 0 );
    assert( std::strcmp( i18n.get( StringId::HumidityPrefix ), "HUM:" ) == 0 );

    bool bad = i18n.setLanguageCode( "fr" );
    assert( !bad );
    assert( i18n.getLanguage() == Language::English );
    std::cout << "PASS: test_language_switching\n";
}

void test_all_string_ids_non_null()
{
    auto &i18n = I18n::instance();
    for( uint8_t langIdx = 0; langIdx <= 1; ++langIdx ) {
        i18n.setLanguage( static_cast<Language>(langIdx) );
        for( uint16_t id = 0; id <= static_cast<uint16_t>(StringId::TimerPaused); ++id ) {
            const char *str = i18n.get( static_cast<StringId>(id) );
            assert( str != nullptr );
            assert( std::strlen( str ) > 0 );
        }
    }
    std::cout << "PASS: test_all_string_ids_non_null\n";
}

void test_builtin_aphorisms()
{
    auto &i18n = I18n::instance();
    i18n.setLanguage( Language::English );
    assert( i18n.getBuiltinAphorismCount() == 25 );
    for( size_t i = 0; i < i18n.getBuiltinAphorismCount(); ++i ) {
        const char *aphorism = i18n.getBuiltinAphorism( i );
        assert( aphorism != nullptr );
        assert( std::strlen( aphorism ) > 0 );
    }

    i18n.setLanguage( Language::Italian );
    assert( i18n.getBuiltinAphorismCount() == 25 );
    for( size_t i = 0; i < i18n.getBuiltinAphorismCount(); ++i ) {
        const char *aphorism = i18n.getBuiltinAphorism( i );
        assert( aphorism != nullptr );
        assert( std::strlen( aphorism ) > 0 );
    }
    std::cout << "PASS: test_builtin_aphorisms\n";
}

void test_oracle_strings()
{
    auto &i18n = I18n::instance();
    i18n.init( Language::Italian );

    assert( std::string( i18n.get( StringId::OracleHeader ) ) == "ORACOLO" );
    assert( std::string( i18n.get( StringId::OraclePromptSpin ) ) == ">> PREMI O AGITA <<" );
    assert( std::string( i18n.get( StringId::OraclePromptRetry ) ) == "[ AGITA / PREMI ]" );
    assert( std::string( i18n.getOracleCategory( 0 ) ) == "Cosa mangiamo?" );
    assert( std::string( i18n.getOracleCategory( 1 ) ) == "Chi ha ragione?" );
    assert( std::string( i18n.getOracleCategory( 2 ) ) == "Cosa mi tocca?" );
    assert( std::string( i18n.getOracleCategory( 3 ) ) == "Cosa guardiamo?" );
    assert( std::string( i18n.getOracleCategory( 4 ) ) == "" );

    const auto &itFoodVerdicts = i18n.getOracleVerdicts( 0 );
    assert( itFoodVerdicts.size() == 5 );
    assert( std::string( itFoodVerdicts[ 0 ] ) == "Pizza" );
    assert( std::string( itFoodVerdicts[ 1 ] ) == "Pasta" );
    assert( std::string( itFoodVerdicts[ 2 ] ) == "Sceglie Lei" );
    assert( std::string( itFoodVerdicts[ 3 ] ) == "Sceglie Lui" );
    assert( std::string( itFoodVerdicts[ 4 ] ) == "Asporto" );

    const auto &itRightVerdicts = i18n.getOracleVerdicts( 1 );
    assert( itRightVerdicts.size() == 3 );
    assert( std::string( itRightVerdicts[ 0 ] ) == "Ha Ragione Lei" );
    assert( std::string( itRightVerdicts[ 1 ] ) == "Ha Ragione Lui" );
    assert( std::string( itRightVerdicts[ 2 ] ) == "Compromesso col Vino" );

    const auto &itChoresVerdicts = i18n.getOracleVerdicts( 2 );
    assert( itChoresVerdicts.size() == 4 );
    assert( std::string( itChoresVerdicts[ 0 ] ) == "Lavare i Piatti" );
    assert( std::string( itChoresVerdicts[ 1 ] ) == "Pattumiera" );
    assert( std::string( itChoresVerdicts[ 2 ] ) == "Portare il Cane" );
    assert( std::string( itChoresVerdicts[ 3 ] ) == "Tutti sul Divano" );

    const auto &itEntertainmentVerdicts = i18n.getOracleVerdicts( 3 );
    assert( itEntertainmentVerdicts.size() == 4 );
    assert( std::string( itEntertainmentVerdicts[ 0 ] ) == "Film" );
    assert( std::string( itEntertainmentVerdicts[ 1 ] ) == "Serie TV" );
    assert( std::string( itEntertainmentVerdicts[ 2 ] ) == "Documentario" );
    assert( std::string( itEntertainmentVerdicts[ 3 ] ) == "A Letto alle 21:30" );

    const auto &itInvalidVerdicts = i18n.getOracleVerdicts( 4 );
    assert( itInvalidVerdicts.empty() );

    i18n.init( Language::English );
    assert( std::string( i18n.get( StringId::OracleHeader ) ) == "ORACLE" );
    assert( std::string( i18n.get( StringId::OraclePromptSpin ) ) == ">> PRESS OR SHAKE <<" );
    assert( std::string( i18n.get( StringId::OraclePromptRetry ) ) == "[ SHAKE / PRESS ]" );
    assert( std::string( i18n.getOracleCategory( 0 ) ) == "What to eat?" );
    assert( std::string( i18n.getOracleCategory( 1 ) ) == "Who's right?" );
    assert( std::string( i18n.getOracleCategory( 2 ) ) == "What to do?" );
    assert( std::string( i18n.getOracleCategory( 3 ) ) == "What to watch?" );
    assert( std::string( i18n.getOracleCategory( 4 ) ) == "" );

    const auto &enFoodVerdicts = i18n.getOracleVerdicts( 0 );
    assert( enFoodVerdicts.size() == 5 );
    assert( std::string( enFoodVerdicts[ 0 ] ) == "Pizza" );
    assert( std::string( enFoodVerdicts[ 1 ] ) == "Pasta" );
    assert( std::string( enFoodVerdicts[ 2 ] ) == "Her Choice" );
    assert( std::string( enFoodVerdicts[ 3 ] ) == "His Choice" );
    assert( std::string( enFoodVerdicts[ 4 ] ) == "Takeout" );

    const auto &enRightVerdicts = i18n.getOracleVerdicts( 1 );
    assert( enRightVerdicts.size() == 3 );
    assert( std::string( enRightVerdicts[ 0 ] ) == "She's Right" );
    assert( std::string( enRightVerdicts[ 1 ] ) == "He's Right" );
    assert( std::string( enRightVerdicts[ 2 ] ) == "Wine Compromise" );

    const auto &enChoresVerdicts = i18n.getOracleVerdicts( 2 );
    assert( enChoresVerdicts.size() == 4 );
    assert( std::string( enChoresVerdicts[ 0 ] ) == "Wash Dishes" );
    assert( std::string( enChoresVerdicts[ 1 ] ) == "Take Out Trash" );
    assert( std::string( enChoresVerdicts[ 2 ] ) == "Walk the Dog" );
    assert( std::string( enChoresVerdicts[ 3 ] ) == "Couch for Both" );

    const auto &enEntertainmentVerdicts = i18n.getOracleVerdicts( 3 );
    assert( enEntertainmentVerdicts.size() == 4 );
    assert( std::string( enEntertainmentVerdicts[ 0 ] ) == "Movie" );
    assert( std::string( enEntertainmentVerdicts[ 1 ] ) == "TV Series" );
    assert( std::string( enEntertainmentVerdicts[ 2 ] ) == "Documentary" );
    assert( std::string( enEntertainmentVerdicts[ 3 ] ) == "Sleep at 9:30 PM" );

    const auto &enInvalidVerdicts = i18n.getOracleVerdicts( 4 );
    assert( enInvalidVerdicts.empty() );

    std::cout << "PASS: test_oracle_strings\n";
}

int main()
{
    std::cout << "Running I18n Native Tests...\n";
    test_default_language();
    test_slap_gag_strings();
    test_language_switching();
    test_all_string_ids_non_null();
    test_builtin_aphorisms();
    test_oracle_strings();
    std::cout << "✅ ALL I18N TESTS PASSED!\n";
    return 0;
}
