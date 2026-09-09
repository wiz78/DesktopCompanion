//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SENTENCE_ENGINE_H
#define FRIENDLYBOT_SENTENCE_ENGINE_H

#include "i18n.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SentenceRef
{
    uint32_t offset = 0;
    uint16_t length = 0;
};

class SentenceEngine
{
public:
    static constexpr size_t MAX_FILE_SIZE_BYTES = 110 * 1024; // 110 KB
    static constexpr const char *SENTENCES_FILE_PATH = "/sentences.json";
    static constexpr const char *SENTENCES_TMP_PATH = "/sentences.tmp";

    SentenceEngine();

    bool begin();
    bool reload();
    void clearCustomSentences();

    [[nodiscard]] bool hasCustomSentences() const;
    [[nodiscard]] size_t getSentenceCount() const;

    bool getNextSentence( std::string& outSentence );

    [[nodiscard]] bool validateQuota( size_t bytes ) const;
    bool saveCustomJsonAtomic( const char *jsonContent, size_t length );

    // Testing harness hook for host-native file simulation
    void initMockEnvironment( const std::string& mockFilePath );

private:
    std::string activeFilePath = SENTENCES_FILE_PATH;
    std::vector<SentenceRef> customIndex;
    std::vector<uint16_t> deck;
    size_t deckIndex = 0;
    bool isCustomActive = false;

    bool scanFileAndBuildIndex( const std::string& path, std::vector<SentenceRef>& outIndex );
    void rebuildDeck();
    void shuffleDeck();
    bool readCustomSentence( const SentenceRef& ref, std::string& out );
    static std::string unescapeJsonString( const std::string& input );
};

#endif // FRIENDLYBOT_SENTENCE_ENGINE_H
