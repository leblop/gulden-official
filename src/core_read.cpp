// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"

#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "serialize.h"
#include "streams.h"
#include <univalue.h>
#include "util.h"
#include "utilstrencodings.h"
#include "version.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>

using namespace std;

CScript ParseScript(const std::string& s)
{
    CScript result;

    static map<string, opcodetype> mapOpNames;

    if (mapOpNames.empty()) {
        for (int op = 0; op <= OP_NOP10; op++) {

            if (op < OP_NOP && op != OP_RESERVED)
                continue;

            const char* name = GetOpName((opcodetype)op);
            if (strcmp(name, "OP_UNKNOWN") == 0)
                continue;
            string strName(name);
            mapOpNames[strName] = (opcodetype)op;

            boost::algorithm::replace_first(strName, "OP_", "");
            mapOpNames[strName] = (opcodetype)op;
        }
    }

    vector<string> words;
    boost::algorithm::split(words, s, boost::algorithm::is_any_of(" \t\n"), boost::algorithm::token_compress_on);

    for (std::vector<std::string>::const_iterator w = words.begin(); w != words.end(); ++w) {
        if (w->empty()) {

        } else if (all(*w, boost::algorithm::is_digit()) || (boost::algorithm::starts_with(*w, "-") && all(string(w->begin() + 1, w->end()), boost::algorithm::is_digit()))) {

            int64_t n = atoi64(*w);
            result << n;
        } else if (boost::algorithm::starts_with(*w, "0x") && (w->begin() + 2 != w->end()) && IsHex(string(w->begin() + 2, w->end()))) {

            std::vector<unsigned char> raw = ParseHex(string(w->begin() + 2, w->end()));
            result.insert(result.end(), raw.begin(), raw.end());
        } else if (w->size() >= 2 && boost::algorithm::starts_with(*w, "'") && boost::algorithm::ends_with(*w, "'")) {

            std::vector<unsigned char> value(w->begin() + 1, w->end() - 1);
            result << value;
        } else if (mapOpNames.count(*w)) {

            result << mapOpNames[*w];
        } else {
            throw runtime_error("script parse error");
        }
    }

    return result;
}

bool DecodeHexTx(CTransaction& tx, const std::string& strHexTx, bool fTryNoWitness)
{
    if (!IsHex(strHexTx))
        return false;

    vector<unsigned char> txData(ParseHex(strHexTx));

    if (fTryNoWitness) {
        CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
        try {
            ssData >> tx;
            if (ssData.eof()) {
                return true;
            }
        }
        catch (const std::exception&) {
        }
    }

    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> tx;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

bool DecodeHexBlk(CBlock& block, const std::string& strHexBlk)
{
    if (!IsHex(strHexBlk))
        return false;

    std::vector<unsigned char> blockData(ParseHex(strHexBlk));
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssBlock >> block;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

uint256 ParseHashUV(const UniValue& v, const string& strName)
{
    string strHex;
    if (v.isStr())
        strHex = v.getValStr();
    return ParseHashStr(strHex, strName); // Note: ParseHashStr("") throws a runtime_error
}

uint256 ParseHashStr(const std::string& strHex, const std::string& strName)
{
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw runtime_error(strName + " must be hexadecimal string (not '" + strHex + "')");

    uint256 result;
    result.SetHex(strHex);
    return result;
}

vector<unsigned char> ParseHexUV(const UniValue& v, const string& strName)
{
    string strHex;
    if (v.isStr())
        strHex = v.getValStr();
    if (!IsHex(strHex))
        throw runtime_error(strName + " must be hexadecimal string (not '" + strHex + "')");
    return ParseHex(strHex);
}
