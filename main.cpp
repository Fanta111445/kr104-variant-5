#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

bool canLogIn(bool password_known, bool needs_code, bool code_correct) {
    if (!password_known) {
        return false;
    }
    if (needs_code) {
        return code_correct;
    }
    return true;
}

bool checkCase(bool password_known, bool code_correct,
               bool expected_password, bool expected_two_step) {
    const bool password = canLogIn(password_known, false, code_correct);
    const bool two_step = canLogIn(password_known, true, code_correct);
    const bool passed = password == expected_password && two_step == expected_two_step;
    std::cout << "password=" << password_known << " code=" << code_correct
              << " | password_only=" << password << " two_step=" << two_step
              << " | " << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--test") {
        const bool test1 = checkCase(false, false, false, false);
        const bool test2 = checkCase(false, true, false, false);
        const bool test3 = checkCase(true, false, true, false);
        const bool test4 = checkCase(true, true, true, true);
        return (test1 && test2 && test3 && test4) ? 0 : 1;
    }

    const bool save_csv = argc == 3 && std::string(argv[1]) == "--csv";
    if (argc != 1 && !save_csv) {
        std::cerr << "Usage: kr104 [--test | --csv PATH]\n";
        return 2;
    }

    std::ofstream csv;
    if (save_csv) {
        csv.open(argv[2]);
        if (!csv) {
            std::cerr << "Cannot open CSV file: " << argv[2] << '\n';
            return 1;
        }
        csv << "password_compromise_pct,owner_code_failure_pct,attempts_per_group,"
            << "outsider_password_successes,outsider_two_step_successes,"
            << "owner_password_successes,owner_two_step_successes,"
            << "outsider_password_pct,outsider_two_step_pct,owner_password_pct,owner_two_step_pct\n"
            << std::fixed << std::setprecision(4);
    }

    const int attempts = 1'000'000;
    const int seed = 104005;
    const int password_chances[] = {1, 5, 10};
    const int owner_failures[] = {0, 10, 20};
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> random_number(1, 1000);

    std::cout << "KR104 variant 5: two-step login simulation\n"
              << "Seed: " << seed << " | Attempts per person and method: " << attempts << "\n\n"
              << "All columns are percentages.\n"
              << " Password  Owner code | Outsider: Outsider: |    Owner:    Owner:\n"
              << "   leaked     failed |  password  two-step |  password  two-step\n"
              << std::fixed << std::setprecision(4);

    for (int password_chance : password_chances) {
        for (int owner_failure : owner_failures) {
            int outsider_password = 0;
            int outsider_two_step = 0;
            int owner_password = 0;
            int owner_two_step = 0;

            for (int attempt = 0; attempt < attempts; ++attempt) {
                // 10 outcomes out of 1000 correspond to 1 percent.
                const bool knows_password_only = random_number(generator) <= password_chance * 10;
                const bool knows_password_two_step = random_number(generator) <= password_chance * 10;
                // Only one outcome out of 1000 is a correct guess.
                const bool guesses_code = knows_password_two_step && random_number(generator) == 1;
                const bool receives_code = random_number(generator) > owner_failure * 10;

                if (canLogIn(knows_password_only, false, false)) {
                    ++outsider_password;
                }
                if (canLogIn(knows_password_two_step, true, guesses_code)) {
                    ++outsider_two_step;
                }
                if (canLogIn(true, false, false)) {
                    ++owner_password;
                }
                if (canLogIn(true, true, receives_code)) {
                    ++owner_two_step;
                }
            }

            const double outsider_password_pct = 100.0 * outsider_password / attempts;
            const double outsider_two_step_pct = 100.0 * outsider_two_step / attempts;
            const double owner_password_pct = 100.0 * owner_password / attempts;
            const double owner_two_step_pct = 100.0 * owner_two_step / attempts;

            std::cout << std::setw(9) << password_chance << std::setw(12) << owner_failure
                      << " |" << std::setw(10) << outsider_password_pct
                      << std::setw(10) << outsider_two_step_pct
                      << " |" << std::setw(10) << owner_password_pct
                      << std::setw(10) << owner_two_step_pct << '\n';

            if (save_csv) {
                csv << password_chance << ',' << owner_failure << ',' << attempts << ','
                    << outsider_password << ',' << outsider_two_step << ','
                    << owner_password << ',' << owner_two_step << ','
                    << outsider_password_pct << ',' << outsider_two_step_pct << ','
                    << owner_password_pct << ',' << owner_two_step_pct << '\n';
            }
        }
    }

    if (save_csv) {
        csv.close();
        if (!csv) {
            std::cerr << "Cannot finish writing CSV file: " << argv[2] << '\n';
            return 1;
        }
    }
    return 0;
}
