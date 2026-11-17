#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string_view>

namespace {

constexpr int attempts = 1'000'000;
constexpr unsigned seed = 104005;
constexpr unsigned scale = 1000;
constexpr unsigned codeGuessChance = 1;  // 0.1%
constexpr std::array<unsigned, 3> passwordChances{10, 50, 100};
constexpr std::array<unsigned, 3> ownerFailureChances{0, 100, 200};

bool canLogIn(bool knowsPassword, bool needsCode, bool passesCode) {
    return knowsPassword && (!needsCode || passesCode);
}

bool randomEvent(std::mt19937& generator, unsigned chance) {
    // Discard the incomplete last block so all 1000 remainders are equally likely.
    constexpr std::uint64_t range = std::uint64_t{std::mt19937::max()} + 1;
    constexpr std::uint64_t bound = range - range % scale;
    std::uint64_t draw;
    do {
        draw = generator();
    } while (draw >= bound);
    return draw % scale < chance;
}

struct SuccessCounts {
    int outsiderPassword = 0;
    int outsiderTwoStep = 0;
    int ownerPassword = 0;
    int ownerTwoStep = 0;
};

SuccessCounts simulate(unsigned passwordChance, unsigned ownerFailureChance,
                       std::mt19937& generator) {
    SuccessCounts counts;
    for (int trial = 0; trial < attempts; ++trial) {
        // The two outsider attempts draw their own password outcomes.
        const bool knowsPasswordOnly = randomEvent(generator, passwordChance);
        const bool knowsPasswordTwoStep = randomEvent(generator, passwordChance);
        const bool guessesCode = knowsPasswordTwoStep && randomEvent(generator, codeGuessChance);
        const bool ownerPassesCode = !randomEvent(generator, ownerFailureChance);

        counts.outsiderPassword += canLogIn(knowsPasswordOnly, false, false);
        counts.outsiderTwoStep += canLogIn(knowsPasswordTwoStep, true, guessesCode);
        counts.ownerPassword += canLogIn(true, false, false);
        counts.ownerTwoStep += canLogIn(true, true, ownerPassesCode);
    }
    return counts;
}

double percent(int successes) {
    return 100.0 * successes / attempts;
}

bool checkLoginRules() {
    struct TestCase {
        bool passwordKnown;
        bool codePassed;
        bool expectedPasswordOnly;
        bool expectedTwoStep;
    };
    constexpr std::array<TestCase, 4> cases{{
        {false, false, false, false},
        {false, true,  false, false},
        {true,  false, true,  false},
        {true,  true,  true,  true},
    }};

    bool allPassed = true;
    for (const auto& test : cases) {
        const bool oneStep = canLogIn(test.passwordKnown, false, test.codePassed);
        const bool twoStep = canLogIn(test.passwordKnown, true, test.codePassed);
        const bool passed = oneStep == test.expectedPasswordOnly && twoStep == test.expectedTwoStep;
        allPassed = allPassed && passed;
        std::cout << "password=" << test.passwordKnown << " code=" << test.codePassed
                  << " | password_only=" << oneStep << " two_step=" << twoStep
                  << " | " << (passed ? "PASS" : "FAIL") << '\n';
    }
    return allPassed;
}

void printResults(std::ostream& console, std::ostream* csv) {
    std::mt19937 generator(seed);
    console << "KR104 variant 5: two-step login simulation\n"
            << "Seed: " << seed << " | Attempts per person and method: " << attempts << "\n\n"
            << "All columns are percentages.\n"
            << " Password  Owner code | Outsider: Outsider: |    Owner:    Owner:\n"
            << "   leaked     failed |  password  two-step |  password  two-step\n"
            << std::fixed;

    if (csv) {
        *csv << "password_compromise_pct,owner_code_failure_pct,attempts_per_group,"
             << "outsider_password_successes,outsider_two_step_successes,"
             << "owner_password_successes,owner_two_step_successes,"
             << "outsider_password_pct,outsider_two_step_pct,owner_password_pct,owner_two_step_pct\n"
             << std::fixed << std::setprecision(4);
    }

    for (const auto passwordChance : passwordChances) {
        for (const auto failureChance : ownerFailureChances) {
            const auto result = simulate(passwordChance, failureChance, generator);
            console << std::setw(9) << passwordChance / 10 << std::setw(12) << failureChance / 10
                    << " |" << std::setprecision(4)
                    << std::setw(10) << percent(result.outsiderPassword)
                    << std::setw(10) << percent(result.outsiderTwoStep) << " |"
                    << std::setw(10) << percent(result.ownerPassword)
                    << std::setw(10) << percent(result.ownerTwoStep) << '\n';

            if (csv) {
                *csv << passwordChance / 10 << ',' << failureChance / 10 << ',' << attempts << ','
                     << result.outsiderPassword << ',' << result.outsiderTwoStep << ','
                     << result.ownerPassword << ',' << result.ownerTwoStep << ','
                     << percent(result.outsiderPassword) << ',' << percent(result.outsiderTwoStep) << ','
                     << percent(result.ownerPassword) << ',' << percent(result.ownerTwoStep) << '\n';
            }
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--test") {
        return checkLoginRules() ? 0 : 1;
    }

    if (argc == 1) {
        printResults(std::cout, nullptr);
        return 0;
    }

    if (argc == 3 && std::string_view(argv[1]) == "--csv") {
        std::ofstream csv(argv[2]);
        if (!csv) {
            std::cerr << "Cannot open CSV file: " << argv[2] << '\n';
            return 1;
        }
        printResults(std::cout, &csv);
        csv.close();
        if (!csv) {
            std::cerr << "Cannot finish writing CSV file: " << argv[2] << '\n';
            return 1;
        }
        return 0;
    }

    std::cerr << "Usage: kr104 [--test | --csv PATH]\n";
    return 2;
}
