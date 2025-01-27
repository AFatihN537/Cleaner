#include "Cleaner.h"
#include "Global.h"

namespace VoteClean {

bool hasVote     = false;
bool canVote     = true;
int  playerCount = 0;

std::unordered_map<mce::UUID, bool> voteList;

int getPlayerCount() {
    int result = 0;
    ll::service::getLevel()->forEachPlayer([&](Player& pl) -> bool {
        if (!pl.isSimulatedPlayer()) {
            result++;
        }
        return true;
    });
    return result;
}

void sendVoteForm(Player* pl) {
    auto fm = ll::form::ModalForm(
        tr("Vote Cleaner"),
        tr("%1$s mengusulkan vote untuk menjalankan Cleaner.\n\n Setuju untuk mulai membersihkan entity sekarang?", {pl->getRealName()}),
        tr("Setuju"),
        tr("Tidak")
    );
    ll::service::getLevel()->forEachPlayer([&](Player& pl) -> bool {
        fm.sendTo(pl, [](Player& player, ll::form::ModalFormResult result, ll::form::FormCancelReason reason) {
            if (result.has_value()) {
                switch (result.value()) {
                case ll::form::ModalFormSelectedButton::Upper: {
                    voteList[player.getUuid()] = true;
                    player.sendMessage(tr("Kamu setuju untuk bersih-bersih entity"));
                    return;
                }
                case ll::form::ModalFormSelectedButton::Lower: {
                    voteList[player.getUuid()] = false;
                    player.sendMessage(tr("Kamu tidak setuju untuk bersih-bersih entity"));
                    return;
                }
                default:
                    return;
                }
            }
        });
        return true;
    });
}

void checkVote() {
    auto& config     = Cleaner::Entry::getInstance().getConfig();
    float percentage = config.VoteClean.Percentage / 100.0f;
    int   voteCount  = 0;
    for (auto& key : voteList) {
        if (key.second == true) {
            voteCount++;
        }
    }
    float result = ((float)voteCount) / ((float)playerCount);
    if (result >= percentage) {
        if (config.Basic.SendBroadcast) {
            Helper::broadcastMessage(tr("Vote selesai!"));
        }
        if (config.Basic.SendToast) {
            Helper::broadcastToast(tr("Vote selesai!"));
        }
        Cleaner::CleanTask();
    } else {
        if (config.Basic.SendBroadcast) {
            Helper::broadcastMessage(tr("Vote gagal!"));
        }
        if (config.Basic.SendToast) {
            Helper::broadcastToast(tr("Vote gagal!"));
        }
    }
    hasVote     = false;
    playerCount = 0;
}

void voteClean(Player* pl) {
    auto& config = Cleaner::Entry::getInstance().getConfig();
    voteList.clear();
    canVote     = false;
    hasVote     = true;
    playerCount = getPlayerCount();
    if (config.Basic.SendBroadcast) {
        Helper::broadcastMessage(tr("%1$s mengusulkan vote untuk menjalankan Cleaner. Kalau kamu ingin ikut vote tapi form vote nya tidak muncul, silahkan ketik /voteclean untuk ikut vote.", {pl->getRealName()}));
    }
    if (config.Basic.SendToast) {
        Helper::broadcastToast(tr("%1$s mengusulkan vote untuk menjalankan Cleaner. Kalau kamu ingin ikut vote tapi form vote nya tidak muncul, silahkan ketik /voteclean untuk ikut vote.", {pl->getRealName()}));
    }
    sendVoteForm(pl);
    ll::coro::keepThis([&config]() -> ll::coro::CoroTask<> {
        co_await std::chrono::seconds::duration(config.VoteClean.Cooldown);
        canVote = true;
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
    ll::coro::keepThis([&config]() -> ll::coro::CoroTask<> {
        co_await std::chrono::seconds::duration(config.VoteClean.CheckDelay);
        checkVote();
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void confirmForm(Player* pl) {
    auto fm = ll::form::ModalForm(
        tr("Vote Cleaner"),
        tr("Apakah kamu ingin mengusulkan vote untuk menjalankan Cleaner?"),
        tr("Okeh"),
        tr("Nggak dulu")
    );
    fm.sendTo(*pl, [](Player& player, ll::form::ModalFormResult result, ll::form::FormCancelReason reason) {
        if (result.has_value()) {
            switch (result.value()) {
            case ll::form::ModalFormSelectedButton::Upper: {
                return voteClean(&player);
            }
            case ll::form::ModalFormSelectedButton::Lower: {
                return player.sendMessage(tr("Vote dibatalkan!"));
            }
            default:
                return;
            }
        }
    });
}

void voteCommandExecute(Player* pl) {
    if (!hasVote) {
        if (canVote) {
            confirmForm(pl);
        } else {
            pl->sendMessage(tr("Vote Cleaner sedang cooldown..."));
        }
    } else {
        if (voteList.count(pl->getUuid())) {
            pl->sendMessage(tr("Kamu sudah memilih!"));
        } else {
            voteList[pl->getUuid()] = true;
            pl->sendMessage(tr("Kamu setuju untuk bersih-bersih entity"));
        }
    }
}

} // namespace VoteClean