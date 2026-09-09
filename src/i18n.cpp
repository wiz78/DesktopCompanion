//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#include "i18n.h"

#include <cstring>

#if __has_include( <Preferences.h> )
#include <Preferences.h>
#endif

namespace
{
    constexpr const char *STRINGS_EN[ ] = {
        // System & Common
        "FRIENDLY BOT", "v50.0", "Wi-Fi Setup", "Factory Reset", "USB Power", "Battery",

        // Mode A: Clock, Weather, Environmental
        "I:", "TEMP:", "HUM:", "Syncing...", "Offline",

        // Mode B: Percussive Maintenance (Slap Gag)
        "ERROR 50", ">> SMACK UNIT! <<", "SIGNAL LOST!", "NO! SLAP IT!", "CALIBRATION COMPLETE", "Sensors aligned",

        // Mode C: Couple Arbitrator (Oracle)
        "ORACLE", ">> PRESS OR SHAKE <<", "[ SHAKE / PRESS ]", "What to eat?", "Who's right?", "What to do?",
        "What to watch?",

        // Mode D: Kitchen Timer
        "Kitchen Timer", "TIME'S UP!", "m", "s", "PAUSED"
    };

    constexpr const char *STRINGS_IT[ ] = {
        // System & Common
        "FRIENDLY BOT", "v50.0", "Config Wi-Fi", "Reset Fabbrica", "Alim. USB", "Batteria",

        // Mode A: Clock, Weather, Environmental
        "I:", "TEMP:", "UMID:", "Sincronizzo...", "Non in linea",

        // Mode B: Percussive Maintenance (Slap Gag)
        "ERRORE 50", ">> DAI UNA BOTTA! <<", "SEGNALE PERSO!", "NO! DAI UNA BOTTA!", "CALIBRAZIONE ESEGUITA",
        "Sensori allineati",

        // Mode C: Couple Arbitrator (Oracle)
        "ORACOLO", ">> PREMI O AGITA <<", "[ AGITA / PREMI ]", "Cosa mangiamo?", "Chi ha ragione?", "Cosa mi tocca?",
        "Cosa guardiamo?",

        // Mode D: Kitchen Timer
        "Timer Cucina", "TEMPO SCADUTO!", "m", "s", "IN PAUSA"
    };

    static_assert( sizeof( STRINGS_EN ) / sizeof( STRINGS_EN[ 0 ] ) == static_cast<size_t>(StringId::TimerPaused) + 1,
                   "STRINGS_EN size does not match StringId count" );
    static_assert( sizeof( STRINGS_IT ) / sizeof( STRINGS_IT[ 0 ] ) == static_cast<size_t>(StringId::TimerPaused) + 1,
                   "STRINGS_IT size does not match StringId count" );

    constexpr const char *APHORISMS_EN[ ] = {
        "Step closer! We know you lost your reading glasses again", "Joints: Crunchy",
        "Firmware v50.0: Leaving parties at 9 PM unlocked", "Patience: 12% - Needs Coffee",
        "Experience Level: Maxed Out", "Couch Mode: High Priority",
        "Birthdays are good for you. Statistics show that the people who have the most live the longest. - Larry Lorenzoni",
        "There is still no cure for the common birthday. - John Glenn",
        "Growing old is mandatory. Growing up is optional. - Chili Davis",
        "You know you're getting old when the candles cost more than the cake. - Bob Hope",
        "The secret of staying young is to live honestly, eat slowly, and lie about your age. - Lucille Ball",
        "A diplomat is a man who always remembers a woman's birthday, but never remembers her age. - Robert Frost",
        "Old age is always fifteen years older than I am. - Bernard Baruch",
        "Aging seems to be the only available way to live a long life. - Kitty O'Neill Collins",
        "I don't feel old. I don't feel anything until noon. Then it's time for my nap. - Bob Hope",
        "You are only young once, but you can stay immature indefinitely. - Ogden Nash",
        "Old age isn't so bad if you consider the alternative. - Maurice Chevalier",
        "We don't grow older, we grow riper. - Pablo Picasso",
        "Getting older is no problem. You just have to live long enough. - Groucho Marx",
        "With age comes wisdom, but sometimes age comes alone. - Oscar Wilde",
        "Inside every old person is a younger person wondering what happened. - Terry Pratchett",
        "At 50, everyone has the face he deserves. - George Orwell",
        "Looking fifty is great - if you are sixty. - Joan Rivers",
        "It takes a long time to become young. - Mark Twain", "I have a great memory, it's just incredibly short."
    };

    constexpr const char *APHORISMS_IT[ ] = {
        "Avvicinati pure, tanto a 50 anni non leggi da lontano!", "Schiena: Croccante",
        "Firmware v50.0: Uscite dopo le 21 disattivate", "Pazienza: 12% - Serve Caffe'", "Livello Esperienza: Massimo",
        "Divano: Modalita' Prioritaria",
        "I compleanni fanno bene. Le statistiche dimostrano che chi ne festeggia di piu' vive piu' a lungo. - Larry Lorenzoni",
        "Non c'e' ancora nessuna cura per il comune compleanno. - John Glenn",
        "Invecchiare e' obbligatorio. Crescere e' facoltativo. - Chili Davis",
        "Sai che stai invecchiando quando le candeline costano piu' della torta. - Bob Hope",
        "Il segreto per restare giovani e' vivere onestamente, mangiare lentamente e mentire sull'eta'. - Lucille Ball",
        "Un diplomatico e' colui che ricorda sempre il compleanno di una donna, ma mai la sua eta'. - Robert Frost",
        "La vecchiaia ha sempre quindici anni piu' di me. - Bernard Baruch",
        "Invecchiare sembra essere l'unico modo per vivere a lungo. - Kitty O'Neill Collins",
        "Non mi sento vecchio. Non sento nulla fino a mezzogiorno, poi e' ora del pisolino. - Bob Hope",
        "Si e' giovani una volta sola, ma si puo' restare immaturi per sempre. - Ogden Nash",
        "La vecchiaia non e' poi cosi male se consideri l'alternativa. - Maurice Chevalier",
        "Non invecchiamo, diventiamo piu' maturi. - Pablo Picasso",
        "Invecchiare non e' un problema. Basta vivere abbastanza a lungo. - Groucho Marx",
        "Con l'eta' arriva la saggezza, ma a volte l'eta' arriva da sola. - Oscar Wilde",
        "Dentro ogni persona anziana c'e' una persona piu' giovane che si chiede cosa sia successo. - Terry Pratchett",
        "A 50 anni ognuno ha la faccia che si merita. - George Orwell",
        "Dimostrare cinquant'anni e' fantastico - se ne hai sessanta. - Joan Rivers",
        "Ci vuole molto tempo per diventare giovani. - Mark Twain",
        "Ho un'ottima memoria, e' solo incredibilmente corta."
    };

    const std::vector<const char *> VERDICTS_EN[ ] = {
        { "Pizza", "Pasta", "Her Choice", "His Choice", "Takeout" }, { "She's Right", "He's Right", "Wine Compromise" },
        { "Wash Dishes", "Take Out Trash", "Walk the Dog", "Couch for Both" },
        { "Movie", "TV Series", "Documentary", "Sleep at 9:30 PM" }
    };

    const std::vector<const char *> VERDICTS_IT[ ] = {
        { "Pizza", "Pasta", "Sceglie Lei", "Sceglie Lui", "Asporto" },
        { "Ha Ragione Lei", "Ha Ragione Lui", "Compromesso col Vino" },
        { "Lavare i Piatti", "Pattumiera", "Portare il Cane", "Tutti sul Divano" },
        { "Film", "Serie TV", "Documentario", "A Letto alle 21:30" }
    };
} // namespace

I18n& I18n::instance()
{
    static I18n inst;
    return inst;
}

void I18n::init( const Language defaultLang )
{
    currentLang = defaultLang;
    loadFromNvs();
}

void I18n::loadFromNvs()
{
#if __has_include( <Preferences.h> )
    if( Preferences prefs; prefs.begin( "friendlybot", true ) ) {
        if( const uint8_t savedLang = prefs.getUChar( "lang", static_cast<uint8_t>(currentLang.load()) );
            savedLang <= static_cast<uint8_t>(Language::Italian) ) {
            currentLang = static_cast<Language>(savedLang);
        }
        prefs.end();
    }
#endif
}

void I18n::saveToNvs()
{
#if __has_include( <Preferences.h> )
    if( Preferences prefs; prefs.begin( "friendlybot", false ) ) {
        prefs.putUChar( "lang", static_cast<uint8_t>(currentLang.load()) );
        prefs.end();
    }
#endif
}

void I18n::setLanguage( const Language lang )
{
    currentLang = lang;
}

bool I18n::setLanguageCode( const char *code )
{
    if( code == nullptr ) {
        return false;
    }

    if( std::strcmp( code, "en" ) == 0 ) {
        setLanguage( Language::English );
        return true;
    }

    if( std::strcmp( code, "it" ) == 0 ) {
        setLanguage( Language::Italian );
        return true;
    }

    return false;
}

Language I18n::getLanguage() const
{
    return currentLang;
}

const char *I18n::getLanguageCode() const
{
    if( currentLang == Language::Italian ) {
        return "it";
    }

    return "en";
}

const char *I18n::get( const StringId id ) const
{
    const auto idx = static_cast<size_t>(id);

    if( currentLang == Language::Italian ) {
        if( idx < std::size( STRINGS_IT ) ) {
            return STRINGS_IT[ idx ];
        }
    } else if( idx < std::size( STRINGS_EN ) ) {
        return STRINGS_EN[ idx ];
    }

    return "";
}

const char *I18n::getOracleCategory( const uint8_t index ) const
{
    switch( index ) {
        case 0:
            return get( StringId::OracleFood );
        case 1:
            return get( StringId::OracleRight );
        case 2:
            return get( StringId::OracleChores );
        case 3:
            return get( StringId::OracleEntertainment );
        default:
            return "";
    }
}

const std::vector<const char *>& I18n::getOracleVerdicts( const uint8_t categoryIndex ) const
{
    static const std::vector<const char *> empty;
    if( categoryIndex >= 4 ) {
        return empty;
    }

    if( currentLang == Language::Italian ) {
        return VERDICTS_IT[ categoryIndex ];
    }

    return VERDICTS_EN[ categoryIndex ];
}

size_t I18n::getBuiltinAphorismCount() const
{
    if( currentLang == Language::Italian ) {
        return std::size( APHORISMS_IT );
    }

    return std::size( APHORISMS_EN );
}

const char *I18n::getBuiltinAphorism( const size_t index ) const
{
    if( const size_t count = getBuiltinAphorismCount(); index >= count ) {
        return "";
    }

    if( currentLang == Language::Italian ) {
        return APHORISMS_IT[ index ];
    }

    return APHORISMS_EN[ index ];
}
