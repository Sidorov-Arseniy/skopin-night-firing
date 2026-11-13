#include "skopin/night.hpp"

#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"

#include <unordered_set>

namespace skopin {

namespace {

std::vector<Ware> filterQueue(const std::vector<Ware>& queue, std::vector<Refusal>& refused) {
    std::vector<Ware> valid_queue;
    std::unordered_set<std::string> seen_marks;

    for (const auto& ware : queue) {
        if (!seen_marks.insert(ware.mark).second) {
            refused.push_back(Refusal{ware.mark, "duplicate"});
            continue;
        }
        if (validateWare(ware).has_value()) {
            refused.push_back(Refusal{ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            refused.push_back(Refusal{ware.mark, "thick"});
            continue;
        }

        valid_queue.push_back(ware);
    }

    return valid_queue;
}

std::vector<Ware> formNextBatch(std::vector<Ware>& waiting) {
    std::vector<Ware> batch;
    if (waiting.empty()) {
        return batch;
    }

    Ware first = waiting.front();
    waiting.erase(waiting.begin());
    batch.push_back(first);

    if (first.kind == WareKind::Sagger) {
        return batch;
    }

    const Program target_program = first.program;
    bool skipped = false;
    std::size_t i = 0;

    while (i < waiting.size()) {
        const Ware& candidate = waiting[i];

        if (candidate.program != target_program || candidate.kind == WareKind::Sagger) {
            break;
        }

        const bool can_cut_in = !skipped || (candidate.kind == WareKind::Rush);
        if (can_cut_in && fitsInKiln(batch, candidate)) {
            batch.push_back(candidate);
            waiting.erase(waiting.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            skipped = true;
            ++i;
        }
    }

    return batch;
}

void runBisqueBatch(Kiln& kiln, const std::vector<Ware>& batch, bool next_is_glaze,
                    std::vector<std::string>& passed) {
    if (kiln.door() == Door::Open) {
        kiln.close();
    }
    if (kiln.cone() == Cone::Bisque) {
        kiln.cool();
    }

    kiln.open();
    for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
        kiln.load(*it);
    }
    kiln.close();

    kiln.fire();
    kiln.open();

    for (const auto& ware : batch) {
        kiln.unload(ware.mark);
        passed.push_back(ware.mark);
    }

    if (!next_is_glaze) {
        kiln.close();
        kiln.cool();
    }
}

void runGlazeBatch(Kiln& kiln, const std::vector<Ware>& batch, std::vector<std::string>& passed) {
    if (kiln.door() == Door::Closed) {
        kiln.open();
    }

    for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
        kiln.load(*it);
    }
    kiln.close();

    if (kiln.cone() == Cone::Cold) {
        kiln.fire();
        kiln.fire();
    } else if (kiln.cone() == Cone::Bisque) {
        kiln.fire();
    }

    kiln.cool();
    kiln.cool();

    kiln.open();
    for (const auto& ware : batch) {
        kiln.unload(ware.mark);
        passed.push_back(ware.mark);
    }
    kiln.close();
}

}  // namespace

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::vector<Ware> waiting = filterQueue(queue, report.refused);

    std::vector<std::vector<Ware>> batches;
    while (!waiting.empty()) {
        batches.push_back(formNextBatch(waiting));
    }

    Kiln kiln;
    for (std::size_t i = 0; i < batches.size(); ++i) {
        const auto& batch = batches[i];
        if (batch.front().program == Program::Bisque) {
            const bool next_is_glaze =
                (i + 1 < batches.size()) && (batches[i + 1].front().program == Program::Glaze);
            runBisqueBatch(kiln, batch, next_is_glaze, report.passed);
        } else {
            runGlazeBatch(kiln, batch, report.passed);
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin

