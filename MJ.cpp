#pragma warning (disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#endif

// 玩家結構體
typedef struct {
    char name[20];
    int score;
    int isDealer;   // 是否為親家：1=是，0=否
    int hasRiichi;  // 是否立直：1=是，0=否
} Player;

// 取得基本點 (Basic Points) 幫助計算
int getBasicPoints(int fan, int fu) {
    if (fan >= 5) {
        if (fan == 5) return 2000; // 滿貫
        if (fan == 6 || fan == 7) return 3000; // 跳滿
        if (fan == 8 || fan == 9 || fan == 10) return 4000; // 倍滿
        if (fan == 11 || fan == 12) return 6000; // 三倍滿
        return 8000; // 役滿
    }
    int basic = fu * (int)(pow(2, fan + 2));
    return basic > 2000 ? 2000 : basic;
}

// 飜符計算函數 (榮和點數)
int calculatePoints(int fan, int fu, int isDealer) {
    int basicPoints = getBasicPoints(fan, fu);
    if (basicPoints == 2000 && fan < 5) return isDealer ? 12000 : 8000; // 滿貫上限

    // 無條件進位至百位數
    int points = isDealer ? ceil(basicPoints * 6 / 100.0) * 100 : ceil(basicPoints * 4 / 100.0) * 100;
    return points;
}

// 聽牌罰符計算，回傳親家是否有聽牌 (1=有, 0=無)
int Tenpai(Player players[], int dealerIdx) {
    int player_tenpai[4] = { 0 };
    int count = 0;
    for (int i = 0; i < 4; i++) {
        int flag;
        printf("%s 是否聽牌？ (1=是, 0=否): ", players[i].name);
        scanf("%d", &flag);
        if (flag) {
            player_tenpai[i] = 1;
            count++;
        }
    }

    if (count > 0 && count < 4) {
        int get_points = 3000 / count;
        int pay_points = 3000 / (4 - count);
        for (int i = 0; i < 4; i++) {
            if (player_tenpai[i]) {
                players[i].score += get_points;
            }
            else {
                players[i].score -= pay_points;
            }
        }
    }
    return player_tenpai[dealerIdx];
}

// 子家獲勝更新分數
void updateScores(Player players[], int winner, int points, int isTsumo, int honba, int fan, int fu, int kyotaku) {
    int basicPoints = getBasicPoints(fan, fu);

    if (isTsumo) {
        printf("自摸計算，總點數: %d 點\n", points);
        // 子家自摸：親家付 2 倍基本點，子家付 1 倍基本點，每人各加付本場費 100
        int dealerPayment = ceil(basicPoints * 2 / 100.0) * 100 + honba * 100;
        int childPayment = ceil(basicPoints * 1 / 100.0) * 100 + honba * 100;

        for (int i = 0; i < 4; i++) {
            if (i != winner) {
                int payment = players[i].isDealer ? dealerPayment : childPayment;
                players[i].score -= payment;
                players[winner].score += payment;
            }
        }
    }
    else {
        printf("輸入放槍者編號 (1-4): ");
        int loserIdx;
        scanf("%d", &loserIdx);
        loserIdx--;
        int totalPayment = points + honba * 300;
        printf("榮和計算，放槍者支付: %d 點\n", totalPayment);
        players[loserIdx].score -= totalPayment;
        players[winner].score += totalPayment;
    }
    players[winner].score += kyotaku * 1000;
}

// 親家獲勝更新分數
void updateScoresDealer(Player players[], int winner, int points, int isTsumo, int honba, int fan, int fu, int kyotaku) {
    int basicPoints = getBasicPoints(fan, fu);

    if (isTsumo) {
        printf("自摸計算，總點數: %d 點\n", points);
        // 親家自摸：三家各付 2 倍基本點，每人加付本場費 100
        int payment = ceil(basicPoints * 2 / 100.0) * 100 + honba * 100;
        for (int i = 0; i < 4; i++) {
            if (i != winner) {
                players[i].score -= payment;
                players[winner].score += payment;
            }
        }
    }
    else {
        printf("輸入放槍者編號 (1-4): ");
        int loserIdx;
        scanf("%d", &loserIdx);
        loserIdx--;
        int totalPayment = points + honba * 300;
        printf("榮和計算，放槍者支付: %d 點\n", totalPayment);
        players[loserIdx].score -= totalPayment;
        players[winner].score += totalPayment;
    }
    players[winner].score += kyotaku * 1000;
}

// 顯示當前分數與場況
void displayStatus(Player players[], const char* round, int honba, int kyotaku) {
    printf("\n========== %s 本場數: %d 供托：%d ==========\n", round, honba, kyotaku * 1000);
    for (int i = 0; i < 4; i++) {
        printf("%s (%s): %d 分\n", players[i].name, players[i].isDealer ? "親家" : "子家", players[i].score);
    }
}

// 更換親家
void rotateDealer(Player players[]) {
    for (int i = 0; i < 4; i++) {
        if (players[i].isDealer) {
            players[i].isDealer = 0; // 清除當前親家標記
            players[(i + 1) % 4].isDealer = 1; // 下一位為親家
            break;
        }
    }
}

// 更新場次字串
void updateRoundString(char* round, int currentDealer) {
    int seat = (currentDealer - 1) % 4 + 1; // 東1局、東2局...
    int wind = (currentDealer - 1) / 4;    // 東風場 (0)、南風場 (1)

    if (wind == 0)
        sprintf(round, "東%d局", seat);
    else if (wind == 1)
        sprintf(round, "南%d局", seat);
    else
        sprintf(round, "西%d局", seat);
}

// 檢查遊戲是否結束
bool checkGameOver(Player players[], int currentDealer) {
    for (int i = 0; i < 4; i++) {
        if (players[i].score < 0) {
            printf("\n遊戲結束！%s 被擊飛！\n", players[i].name);
            return true;
        }
    }
    if (currentDealer > 8) { // 東南戰（8局）結束
        printf("\n遊戲結束！打滿局數。\n");
        return true;
    }
    return false;
}

// 主程式
int main() {

#ifdef _WIN32
    SetConsoleOutputCP(950);
    SetConsoleCP(950);
#endif

    printf("對局開始！\n\n");
    Player players[4] = {
        {"玩家1", 25000, 1, 0}, // 初始親家
        {"玩家2", 25000, 0, 0},
        {"玩家3", 25000, 0, 0},
        {"玩家4", 25000, 0, 0}
    };

    int honba = 0; // 本場數
    int kyotaku = 0; // 供托
    int currentDealer = 1; // 總局數（東1局起始）
    char round[15];
    sprintf(round, "東1局");

    while (!checkGameOver(players, currentDealer)) {
        displayStatus(players, round, honba, kyotaku);

        // 檢查玩家是否立直
        for (int i = 0; i < 4; i++) {
            if (!players[i].hasRiichi && players[i].score >= 1000) {
                int choice;
                printf("%s 是否立直？ (1=是, 0=否): ", players[i].name);
                scanf("%d", &choice);
                if (choice == 1) {
                    players[i].score -= 1000;
                    kyotaku++;
                    players[i].hasRiichi = 1;
                }
            }
        }

        // 確認為流局或有玩家獲勝
        printf("\n輸入贏家編號 (1-4)，流局輸入0: ");
        int winner;
        scanf("%d", &winner);

        // ==== 處理流局 ====
        if (winner == 0) {
            int dealerIdx = 0;
            for (int i = 0; i < 4; i++) { if (players[i].isDealer) dealerIdx = i; }

            int isDealerTenpai = Tenpai(players, dealerIdx);
            honba++; // 流局本場數必定 +1
            printf("\n流局！本場數增加為 %d，供托累積: %d\n", honba, kyotaku * 1000);

            for (int i = 0; i < 4; i++) players[i].hasRiichi = 0; // 重置立直

            if (!isDealerTenpai) { // 親家未聽牌，輪替親家，但保留本場數
                rotateDealer(players);
                currentDealer++;
                updateRoundString(round, currentDealer);
            }
            continue; // 直接進入下一局
        }

        // ==== 處理和牌 ====
        winner--;

        printf("輸入番數: ");
        int fan;
        scanf("%d", &fan);

        int fu = 20; // 修正變數遮蔽問題
        if (fan < 5) {
            printf("輸入符數: ");
            scanf("%d", &fu);
        }

        printf("是否為自摸？ (1=是, 0=否): ");
        int isTsumo;
        scanf("%d", &isTsumo);

        int points = calculatePoints(fan, fu, players[winner].isDealer);
        bool isDealerWin = players[winner].isDealer;

        if (!isDealerWin) {
            updateScores(players, winner, points, isTsumo, honba, fan, fu, kyotaku);
            honba = 0; // 子家和牌，本場重置
            kyotaku = 0;
            rotateDealer(players);
            currentDealer++;
        }
        else {
            updateScoresDealer(players, winner, points, isTsumo, honba, fan, fu, kyotaku);
            honba++; // 親家和牌，連莊，本場數+1
            kyotaku = 0;
        }

        updateRoundString(round, currentDealer);

        for (int i = 0; i < 4; i++) {
            players[i].hasRiichi = 0;
        }
    }

    printf("\n========== 最終分數 ==========\n");
    displayStatus(players, round, honba, kyotaku);
    printf("感謝遊玩！\n");
    printf("作者：劉文傑\n");

    // 暫停畫面，避免執行完馬上關閉視窗
    system("pause");
    return 0;
}