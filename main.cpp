#include <iostream>
#include <string>
#include "client.hpp"
#include "queue/working_queue.h"
#include "device_manager.hpp"

#include "debug.hpp"
#include "hw_definitions.hpp"

int main()
{
    Client client;
    std::vector<std::unique_ptr<DeviceManager>> trezors;
    std::vector<std::unique_ptr<std::atomic_flag>> is_alive_flags;

    auto enumerates = client.enumerate();

    if (enumerates.size() == 0)
    {
        std::cout << "there is no device connected" << std::endl;
        return 1;
    }

    auto clear_flag = [&](size_t queue_size, size_t idx) {
        if (queue_size == 0)
            is_alive_flags.at(idx)->clear();
    };

    for (auto enumerate : enumerates)
    {
        trezors.push_back(std::unique_ptr<DeviceManager>(new DeviceManager()));
        {
            auto af = std::make_unique<std::atomic_flag>();
            af->test_and_set();
            is_alive_flags.emplace_back(move(af));
        }
        auto is_alive_idx = is_alive_flags.size() - 1;
        auto& trezor = trezors.back();

        if (enumerate.session != "null")
        {
            client.release(enumerate.session);
            enumerate.session = "null";
        }

        trezor->callback_Failure([&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
            std::cout << "SESSION: " << session << std::endl;
            std::cout << "FAIL REASON: " << child_cast<Message, Failure>(msg).message() << std::endl;
            std::cout << std::endl;
            clear_flag(queue_size, is_alive_idx);
        });

        trezor->callback_Success([&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
            std::cout << "SESSION: " << session << std::endl;
            std::cout << "SUCCESS: " << child_cast<Message, Success>(msg).message() << std::endl;
            std::cout << std::endl;
            clear_flag(queue_size, is_alive_idx);
        });

        try
        {
            using namespace hw::trezor::messages::hds;
            
            trezor->init(enumerate);
            trezor->call_Ping("hello hds", true, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "PONG: " << child_cast<Message, Success>(msg).message() << std::endl;
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });
            trezor->call_HdsGetOwnerKey(true, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HDS OWNER KEY: ";
                print_bin(reinterpret_cast<const uint8_t *>(child_cast<Message, HdsOwnerKey>(msg).key().c_str()), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });
            trezor->call_HdsGenerateNonce(1, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HDS NONCE IN SLOT 1: " << std::endl;
                std::cout << "pub_x: ";
                print_bin(reinterpret_cast<const uint8_t *>(child_cast<Message, HdsECCPoint>(msg).x().c_str()), 32);
                std::cout << "pub_y: " << child_cast<Message, HdsECCPoint>(msg).y() << std::endl;
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });
            trezor->call_HdsGetNoncePublic(1, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HDS PUBLIC KEY OF NONCE IN SLOT 1:" << std::endl;
                std::cout << "pub_x: ";
                print_bin(reinterpret_cast<const uint8_t *>(child_cast<Message, HdsECCPoint>(msg).x().c_str()), 32);
                std::cout << "pub_y: " << child_cast<Message, HdsECCPoint>(msg).y() << std::endl;
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            uint8_t test_bytes[32] = {0x87, 0xdc, 0x3d, 0x21, 0x41, 0x74, 0x82, 0x0e, 0x11, 0x54, 0xb4, 0x9b, 0xc6, 0xcd, 0xb2, 0xab, 0xd4, 0x5e, 0xe9, 0x58, 0x17, 0x05, 0x5d, 0x25, 0x5a, 0xa3, 0x58, 0x31, 0xb7, 0x0d, 0x32, 0x66};
            std::vector<HdsCrypto_CoinID> inputs;
            inputs.push_back({1, 1, 1, 2, 0});
            inputs.push_back({2, 2, 2, 5, 0});
            std::vector<HdsCrypto_CoinID> outputs;
            outputs.push_back({3, 3, 3, 3, 0});

            HdsCrypto_TxCommon txCommon;
            txCommon.m_pIns = &inputs;
            txCommon.m_pOuts = &outputs;
            memcpy(txCommon.m_kOffset.m_pVal, test_bytes, 32);
            txCommon.m_Krn.m_Fee = 20;
            txCommon.m_Krn.m_hMin = 3;
            txCommon.m_Krn.m_hMax = 43;
            memcpy(txCommon.m_Krn.m_Commitment.m_X.m_pVal, test_bytes, 32);
            txCommon.m_Krn.m_Commitment.m_Y = 1;
            memcpy(txCommon.m_Krn.m_Signature.m_k.m_pVal, test_bytes, 32);
            txCommon.m_Krn.m_Signature.m_NoncePub.m_Y = 1;
            memcpy(txCommon.m_Krn.m_Signature.m_NoncePub.m_X.m_pVal, test_bytes, 32);

            HdsCrypto_TxMutualInfo txMutualInfo;
            memcpy(txMutualInfo.m_Peer.m_pVal, test_bytes, 32);
            txMutualInfo.m_MyIDKey = 25;
            memcpy(txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_X.m_pVal, test_bytes, 32);
            txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_Y = 1;
            memcpy(txMutualInfo.m_PaymentProofSignature.m_k.m_pVal, test_bytes, 32);

            HdsCrypto_TxSenderParams txSenderParams;
            txSenderParams.m_iSlot = 2;
            memset(txSenderParams.m_UserAgreement.m_pVal, 0, 32);

            trezor->call_HdsSignTransactionSend(txCommon, txMutualInfo, txSenderParams, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                const uint8_t *offset_sk = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsSignTransactionSend>(msg).tx_common().offset_sk().c_str());
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsSignTransactionSend" << std::endl;
                std::cout << "HDS TX offset_sk: ";
                print_bin(reinterpret_cast<const uint8_t *>(offset_sk), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            trezor->call_HdsSignTransactionReceive(txCommon, txMutualInfo, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                const uint8_t *offset_sk = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsSignTransactionReceive>(msg).tx_common().offset_sk().c_str());
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsSignTransactionReceive" << std::endl;
                std::cout << "HDS TX offset_sk: ";
                print_bin(reinterpret_cast<const uint8_t *>(offset_sk), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            trezor->call_HdsSignTransactionSplit(txCommon, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                const uint8_t *offset_sk = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsSignTransactionSplit>(msg).tx_common().offset_sk().c_str());
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsSignTransactionSplit" << std::endl;
                std::cout << "HDS TX offset_sk: ";
                print_bin(reinterpret_cast<const uint8_t *>(offset_sk), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            HdsCrypto_CoinID cid = {1, 111, 16777216, 23110, 0};
            HdsCrypto_CompactPoint pt0;
            HdsCrypto_CompactPoint pt1;
            hex2bin("5cea50aa1375f9482f605541b1883c9ec8d2206444cc32754d7371f018a44410", 64, pt0.m_X.m_pVal);
            hex2bin("b18f7b5debc52ccb967a3361f0b74cd2aa6102f78cd82a7378870b056c2aa519", 64, pt1.m_X.m_pVal);
            pt0.m_Y = 1;
            pt1.m_Y = 1;

            trezor->call_HdsGenerateRangeproof(&cid, &pt0, &pt1, nullptr, nullptr, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                bool is_successful = child_cast<Message, HdsRangeproofData>(msg).is_successful();
                const uint8_t *pt0_x = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsRangeproofData>(msg).pt0().x().c_str());
                const uint8_t *pt1_x = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsRangeproofData>(msg).pt1().x().c_str());
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsGenerateRangeproof" << std::endl;
                std::cout << "is_successful: " << is_successful << std::endl;
                std::cout << "pt0_x: ";
                print_bin(reinterpret_cast<const uint8_t *>(pt0_x), 32);
                std::cout << "pt0_y: ";
                print_bin(reinterpret_cast<const uint8_t *>(pt1_x), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            trezor->call_HdsGetPKdf(true, 0, true, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                const uint8_t *key = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsPKdf>(msg).key().c_str());
                const uint8_t *cofactor_g_x = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsPKdf>(msg).cofactor_g().x().c_str());
                const uint8_t *cofactor_j_x = reinterpret_cast<const uint8_t *>(child_cast<Message, HdsPKdf>(msg).cofactor_j().x().c_str());
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsGetPKdf" << std::endl;
                std::cout << "key: ";
                print_bin(reinterpret_cast<const uint8_t *>(key), 32);
                std::cout << "cofactor_g_x: ";
                print_bin(reinterpret_cast<const uint8_t *>(cofactor_g_x), 32);
                std::cout << "cofactor_j_x: ";
                print_bin(reinterpret_cast<const uint8_t *>(cofactor_j_x), 32);
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });

            trezor->call_HdsGetNumSlots(true, [&, is_alive_idx](const Message &msg, std::string session, size_t queue_size) {
                uint32_t num_slots = child_cast<Message, HdsNumSlots>(msg).num_slots();
                std::cout << "SESSION: " << session << std::endl;
                std::cout << "HdsGetNumSlots" << std::endl;
                std::cout << "num_slots: " << num_slots << std::endl;
                std::cout << std::endl;
                clear_flag(queue_size, is_alive_idx);
            });
        }
        catch (std::runtime_error e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    for (auto& is_alive : is_alive_flags)
        while (is_alive->test_and_set())
            ; // waiting
    curl_global_cleanup();
    return 0;
}
