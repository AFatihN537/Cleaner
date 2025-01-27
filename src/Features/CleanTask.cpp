#include "Cleaner.h"
#include <memory>

namespace Cleaner {

static std::shared_ptr<bool> mAutoCleanTask  = std::make_shared<bool>(false);
static std::shared_ptr<bool> mCleanTaskCount = std::make_shared<bool>(false);
static std::shared_ptr<bool> mCleanTaskTPS   = std::make_shared<bool>(false);

bool auto_clean_triggerred = false;

void CleanTask() {
    auto& config        = Cleaner::Entry::getInstance().getConfig();
    auto  time          = config.Basic.Notice1;
    auto  announce_time = config.Basic.Notice2;
    auto  time_1        = std::chrono::seconds::duration(time);
    auto  time_2        = std::chrono::seconds::duration(time - announce_time);
    if (config.Basic.ConsoleLog) {
        ll::io::LoggerRegistry::getInstance().getOrCreate("Cleaner")->info(
            tr("Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(time_1.count())})
        );
    }
    if (config.Basic.SendBroadcast) {
        Helper::broadcastMessage(tr("Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(time_1.count())}));
    }
    if (config.Basic.SendToast) {
        Helper::broadcastToast(tr("Perhatian! Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(announce_time)}));
    }
    ll::coro::keepThis([announce_time, &config, time_2]() -> ll::coro::CoroTask<> {
        co_await time_2;
        if (config.Basic.ConsoleLog) {
            ll::io::LoggerRegistry::getInstance().getOrCreate("Cleaner")->info(
                tr("Perhatian! Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(announce_time)})
            );
        }
        if (config.Basic.SendBroadcast) {
            Helper::broadcastMessage(tr("Perhatian! Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(announce_time)}));
        }
        if (config.Basic.SendToast) {
            Helper::broadcastToast(tr("Perhatian! Sistem akan membersihkan entity server secara otomatis dalam %1$s detik", {S(announce_time)}));
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
    ll::coro::keepThis([announce_time, &config, time_1]() -> ll::coro::CoroTask<> {
        co_await time_1;
        auto count = ExecuteClean();
        if (config.Basic.ConsoleLog) {
            ll::io::LoggerRegistry::getInstance().getOrCreate("Cleaner")->info(tr("Pembersihan selesai! %1$s entity telah dihilangkan", {S(count)}));
        }
        if (config.Basic.SendBroadcast) {
            Helper::broadcastMessage(tr("Pembersihan selesai! %1$s entity telah dihilangkan", {S(count)}));
        }
        if (config.Basic.SendToast) {
            Helper::broadcastToast(tr("Pembersihan selesai! %1$s entity telah dihilangkan", {S(count)}));
        }
        auto_clean_triggerred = false;
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void AutoCleanTask(int seconds) {
    auto time = std::chrono::seconds::duration(seconds);
    *mAutoCleanTask = true;
    ll::coro::keepThis([time]() -> ll::coro::CoroTask<> {
        while (true) {
            co_await time;
            if (*mAutoCleanTask == false) co_return;
            CleanTask();
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void CleanTaskCount(int max_entities) {
    auto& config = Cleaner::Entry::getInstance().getConfig();
    *mCleanTaskCount = true;
    ll::coro::keepThis([max_entities, &config]() -> ll::coro::CoroTask<> {
        while (true) {
            co_await 10s;
            if (*mCleanTaskCount == false) co_return;
            auto count = CountEntities();
            if (auto_clean_triggerred == false) {
                if (count >= max_entities) {
                    auto_clean_triggerred = true;
                    if (config.Basic.ConsoleLog) {
                        ll::io::LoggerRegistry::getInstance().getOrCreate("Cleaner")->warn(tr("Entity di server saat ini terlalu banyak! Ada %1$s entity yang dapat dibersihkan. Cleaner akan dijalankan.", {S(count)}));
                    }
                    if (config.Basic.SendBroadcast) {
                        Helper::broadcastMessage(tr("Entity di server saat ini terlalu banyak! Ada %1$s entity yang dapat dibersihkan. Cleaner akan dijalankan.", {S(count)}));
                    }
                    if (config.Basic.SendToast) {
                        Helper::broadcastToast(tr("Entity di server saat ini terlalu banyak! Ada %1$s entity yang dapat dibersihkan. Cleaner akan dijalankan.", {S(count)}));
                    }
                    CleanTask();
                }
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void CleanTaskTPS(float min_tps) {
    auto& config = Cleaner::Entry::getInstance().getConfig();
    *mCleanTaskTPS = true;
    ll::coro::keepThis([min_tps, &config]() -> ll::coro::CoroTask<> {
        while (true) {
            co_await 10s;
            if (*mCleanTaskTPS == false) co_return;
            if (auto_clean_triggerred == false) {
                if (GMLIB_Level::getLevel()->getServerAverageTps() <= min_tps) {
                    auto_clean_triggerred = true;
                    auto mspt             = S(GMLIB_Level::getLevel()->getServerAverageTps());
                    if (config.Basic.ConsoleLog) {
                        ll::io::LoggerRegistry::getInstance().getOrCreate("Cleaner")->warn(
                            tr("TPS rendah terdeteksi! TPS server saat ini %1$s\nCleaner akan dijalankan.", {mspt})
                        );
                    }
                    if (config.Basic.SendBroadcast) {
                        Helper::broadcastMessage(tr("TPS rendah terdeteksi! TPS server saat ini %1$s\nCleaner akan dijalankan.", {mspt}));
                    }
                    if (config.Basic.SendToast) {
                        Helper::broadcastToast(tr("TPS rendah terdeteksi! TPS server saat ini %1$s\nCleaner akan dijalankan.", {mspt}));
                    }
                    CleanTask();
                }
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

void stopAllTasks() {
    if (!mCleanTaskCount) {
        *mCleanTaskTPS = false;
    }
    if (mCleanTaskCount) {
        *mCleanTaskCount = false;
    }
    if (mAutoCleanTask) {
        *mAutoCleanTask = false;
    }
}
} // namespace Cleaner
