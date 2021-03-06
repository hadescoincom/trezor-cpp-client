#pragma once

#include "base_device_manager.hpp"
#include "hw_definitions.hpp"

class DeviceManager: public BaseDeviceManager
{
public:
  virtual ~DeviceManager()
  {
  }

  void call_Ping(std::string text, bool button_protection, MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    Ping message;
    message.set_message(text);
    message.set_button_protection(button_protection);
    call(pack_message(message), MessageType_Success, std::move(callback));
  }

  void call_HdsGetOwnerKey(bool show_display, MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGetOwnerKey message;
    message.set_show_display(show_display);
    call(pack_message(message), MessageType_HdsOwnerKey, std::move(callback));
  }

  void call_HdsGenerateNonce(uint8_t slot, MessageCallback callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGenerateNonce message;
    message.set_slot(slot);
    call(pack_message(message), MessageType_HdsECCPoint, std::move(callback));
  }

  void call_HdsGetNoncePublic(uint8_t slot, MessageCallback callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGetNoncePublic message;
    message.set_slot(slot);
    call(pack_message(message), MessageType_HdsECCPoint, std::move(callback));
  }

  // NEW CRYPTO ---------------------------------------------------------

  void call_HdsGenerateRangeproof(const HdsCrypto_CoinID *cid,
                                   const HdsCrypto_CompactPoint *pt0,
                                   const HdsCrypto_CompactPoint *pt1,
                                   const HdsCrypto_UintBig *extra_sk0,
                                   const HdsCrypto_UintBig *extra_sk1,
                                   MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGenerateRangeproof message;
    auto coinId = message.mutable_cid();
    coinId->set_idx(cid->m_Idx);
    coinId->set_type(cid->m_Type);
    coinId->set_sub_idx(cid->m_SubIdx);
    coinId->set_amount(cid->m_Amount);
    coinId->set_asset_id(cid->m_AssetID);

    auto message_pt0 = message.mutable_pt0();
    message_pt0->set_x(pt0->m_X.m_pVal, 32);
    message_pt0->set_y(pt0->m_Y);

    auto message_pt1 = message.mutable_pt1();
    message_pt1->set_x(pt1->m_X.m_pVal, 32);
    message_pt1->set_y(pt1->m_Y);

    if (extra_sk0)
      message.set_extra_sk0(extra_sk0->m_pVal, 32);

    if (extra_sk1)
      message.set_extra_sk0(extra_sk1->m_pVal, 32);

    call(pack_message(message), MessageType_HdsRangeproofData, std::move(callback));
  }

  void call_HdsSignTransactionSend(const HdsCrypto_TxCommon &txCommon,
                                    const HdsCrypto_TxMutualInfo &txMutualInfo,
                                    const HdsCrypto_TxSenderParams &txSenderParams,
                                    MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsSignTransactionSend message;

    auto tx_common = message.mutable_tx_common();
    tx_common->set_offset_sk(txCommon.m_kOffset.m_pVal, 32);
    for (auto in : *txCommon.m_pIns)
    {
      auto coinId = tx_common->add_inputs();
      coinId->set_idx(in.m_Idx);
      coinId->set_type(in.m_Type);
      coinId->set_sub_idx(in.m_SubIdx);
      coinId->set_amount(in.m_Amount);
      coinId->set_asset_id(in.m_AssetID);
    }
    for (auto out : *txCommon.m_pOuts)
    {
      auto coinId = tx_common->add_outputs();
      coinId->set_idx(out.m_Idx);
      coinId->set_type(out.m_Type);
      coinId->set_sub_idx(out.m_SubIdx);
      coinId->set_amount(out.m_Amount);
      coinId->set_asset_id(out.m_AssetID);
    }
    auto kernel_params = tx_common->mutable_kernel_params();
    kernel_params->set_fee(txCommon.m_Krn.m_Fee);
    kernel_params->set_min_height(txCommon.m_Krn.m_hMin);
    kernel_params->set_max_height(txCommon.m_Krn.m_hMax);
    auto commitment = kernel_params->mutable_commitment();
    commitment->set_x(txCommon.m_Krn.m_Commitment.m_X.m_pVal, 32);
    commitment->set_y(txCommon.m_Krn.m_Commitment.m_Y);
    auto kernel_signature = kernel_params->mutable_signature();
    kernel_signature->set_sign_k(txCommon.m_Krn.m_Signature.m_k.m_pVal, 32);
    auto kernel_nonce_pub = kernel_signature->mutable_nonce_pub();
    kernel_nonce_pub->set_x(txCommon.m_Krn.m_Signature.m_NoncePub.m_X.m_pVal, 32);
    kernel_nonce_pub->set_y(txCommon.m_Krn.m_Signature.m_NoncePub.m_Y);

    auto tx_mutual_info = message.mutable_tx_mutual_info();
    tx_mutual_info->set_peer(txMutualInfo.m_Peer.m_pVal, 32);
    tx_mutual_info->set_wallet_identity_key(txMutualInfo.m_MyIDKey);
    auto payment_proof_signature = tx_mutual_info->mutable_payment_proof_signature();
    payment_proof_signature->set_sign_k(txMutualInfo.m_PaymentProofSignature.m_k.m_pVal, 32);
    auto payment_proof_nonce_pub = payment_proof_signature->mutable_nonce_pub();
    payment_proof_nonce_pub->set_x(txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_X.m_pVal, 32);
    payment_proof_nonce_pub->set_y(txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_Y);

    message.set_nonce_slot(txSenderParams.m_iSlot);
    message.set_user_agreement(txSenderParams.m_UserAgreement.m_pVal, 32);

    call(pack_message(message), MessageType_HdsSignTransactionSend, std::move(callback));
  }

  void call_HdsSignTransactionReceive(const HdsCrypto_TxCommon &txCommon,
                                       const HdsCrypto_TxMutualInfo &txMutualInfo,
                                       MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsSignTransactionReceive message;

    auto tx_common = message.mutable_tx_common();
    tx_common->set_offset_sk(txCommon.m_kOffset.m_pVal, 32);
    for (auto in : *txCommon.m_pIns)
    {
      auto coinId = tx_common->add_inputs();
      coinId->set_idx(in.m_Idx);
      coinId->set_type(in.m_Type);
      coinId->set_sub_idx(in.m_SubIdx);
      coinId->set_amount(in.m_Amount);
      coinId->set_asset_id(in.m_AssetID);
    }
    for (auto out : *txCommon.m_pOuts)
    {
      auto coinId = tx_common->add_outputs();
      coinId->set_idx(out.m_Idx);
      coinId->set_type(out.m_Type);
      coinId->set_sub_idx(out.m_SubIdx);
      coinId->set_amount(out.m_Amount);
      coinId->set_asset_id(out.m_AssetID);
    }
    auto kernel_params = tx_common->mutable_kernel_params();
    kernel_params->set_fee(txCommon.m_Krn.m_Fee);
    kernel_params->set_min_height(txCommon.m_Krn.m_hMin);
    kernel_params->set_max_height(txCommon.m_Krn.m_hMax);
    auto commitment = kernel_params->mutable_commitment();
    commitment->set_x(txCommon.m_Krn.m_Commitment.m_X.m_pVal, 32);
    commitment->set_y(txCommon.m_Krn.m_Commitment.m_Y);
    auto kernel_signature = kernel_params->mutable_signature();
    kernel_signature->set_sign_k(txCommon.m_Krn.m_Signature.m_k.m_pVal, 32);
    auto kernel_nonce_pub = kernel_signature->mutable_nonce_pub();
    kernel_nonce_pub->set_x(txCommon.m_Krn.m_Signature.m_NoncePub.m_X.m_pVal, 32);
    kernel_nonce_pub->set_y(txCommon.m_Krn.m_Signature.m_NoncePub.m_Y);

    auto tx_mutual_info = message.mutable_tx_mutual_info();
    tx_mutual_info->set_peer(txMutualInfo.m_Peer.m_pVal, 32);
    tx_mutual_info->set_wallet_identity_key(txMutualInfo.m_MyIDKey);
    auto payment_proof_signature = tx_mutual_info->mutable_payment_proof_signature();
    payment_proof_signature->set_sign_k(txMutualInfo.m_PaymentProofSignature.m_k.m_pVal, 32);
    auto payment_proof_nonce_pub = payment_proof_signature->mutable_nonce_pub();
    payment_proof_nonce_pub->set_x(txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_X.m_pVal, 32);
    payment_proof_nonce_pub->set_y(txMutualInfo.m_PaymentProofSignature.m_NoncePub.m_Y);

    call(pack_message(message), MessageType_HdsSignTransactionReceive, std::move(callback));
  }

  void call_HdsSignTransactionSplit(const HdsCrypto_TxCommon &txCommon,
                                     MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsSignTransactionSplit message;

    auto tx_common = message.mutable_tx_common();
    tx_common->set_offset_sk(txCommon.m_kOffset.m_pVal, 32);
    for (auto in : *txCommon.m_pIns)
    {
      auto coinId = tx_common->add_inputs();
      coinId->set_idx(in.m_Idx);
      coinId->set_type(in.m_Type);
      coinId->set_sub_idx(in.m_SubIdx);
      coinId->set_amount(in.m_Amount);
      coinId->set_asset_id(in.m_AssetID);
    }
    for (auto out : *txCommon.m_pOuts)
    {
      auto coinId = tx_common->add_outputs();
      coinId->set_idx(out.m_Idx);
      coinId->set_type(out.m_Type);
      coinId->set_sub_idx(out.m_SubIdx);
      coinId->set_amount(out.m_Amount);
      coinId->set_asset_id(out.m_AssetID);
    }
    auto kernel_params = tx_common->mutable_kernel_params();
    kernel_params->set_fee(txCommon.m_Krn.m_Fee);
    kernel_params->set_min_height(txCommon.m_Krn.m_hMin);
    kernel_params->set_max_height(txCommon.m_Krn.m_hMax);
    auto commitment = kernel_params->mutable_commitment();
    commitment->set_x(txCommon.m_Krn.m_Commitment.m_X.m_pVal, 32);
    commitment->set_y(txCommon.m_Krn.m_Commitment.m_Y);
    auto kernel_signature = kernel_params->mutable_signature();
    kernel_signature->set_sign_k(txCommon.m_Krn.m_Signature.m_k.m_pVal, 32);
    auto kernel_nonce_pub = kernel_signature->mutable_nonce_pub();
    kernel_nonce_pub->set_x(txCommon.m_Krn.m_Signature.m_NoncePub.m_X.m_pVal, 32);
    kernel_nonce_pub->set_y(txCommon.m_Krn.m_Signature.m_NoncePub.m_Y);

    call(pack_message(message), MessageType_HdsSignTransactionSplit, std::move(callback));
  }

  void call_HdsGetPKdf(bool is_root_key, uint32_t child_idx, bool show_display, MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGetPKdf message;
    message.set_is_root_key(is_root_key);
    message.set_child_idx(child_idx);
    message.set_show_display(show_display);

    call(pack_message(message), MessageType_HdsPKdf, std::move(callback));
  }

  void call_HdsGetNumSlots(bool show_display, MessageCallback&& callback)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    HdsGetNumSlots message;
    message.set_show_display(show_display);

    call(pack_message(message), MessageType_HdsNumSlots, std::move(callback));
  }

protected:
  virtual void handle_custom_response(const Call &call, const std::string &session)
  {
    using namespace hw::trezor::messages;
    using namespace hw::trezor::messages::hds;

    switch (call.type)
    {
    case MessageType_HdsOwnerKey:
      execute_callback<HdsOwnerKey>(call, session);
      break;
    case MessageType_HdsECCPoint:
      execute_callback<HdsECCPoint>(call, session);
      break;
    case MessageType_HdsRangeproofData:
      execute_callback<HdsRangeproofData>(call, session);
      break;
    case MessageType_HdsSignedTransaction:
      execute_callback<HdsSignedTransaction>(call, session);
      break;
    case MessageType_HdsSignTransactionSend:
      execute_callback<HdsSignTransactionSend>(call, session);
      break;
    case MessageType_HdsSignTransactionReceive:
      execute_callback<HdsSignTransactionReceive>(call, session);
      break;
    case MessageType_HdsSignTransactionSplit:
      execute_callback<HdsSignTransactionSplit>(call, session);
      break;
    case MessageType_HdsPKdf:
      execute_callback<HdsPKdf>(call, session);
      break;
    case MessageType_HdsNumSlots:
      execute_callback<HdsNumSlots>(call, session);
      break;
    default:
      break;
    }
  }
};