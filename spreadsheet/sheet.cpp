#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <stack>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::CorrectSheetSizeToNewPos(Position pos) {

    if (pos.col >= sheet_size_.cols) {
        sheet_size_.cols = pos.col + 1;
    }

    if (pos.row >= sheet_size_.rows) {
        sheet_size_.rows = pos.row + 1;
    }
}

void Sheet::SetCell(Position pos, const std::string& text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("wrong position");
    }

    CorrectSheetSizeToNewPos(pos);

    Cell* cell_ptr = reinterpret_cast<Cell*>(this->GetCell(pos));

    if (cell_ptr != nullptr) {
        if (cell_ptr->GetText() == text) {
            return;
        }

        cell_ptr->Set(text);
    }
    else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);
        sheet_[pos] = std::move(new_cell);

        cell_ptr = reinterpret_cast<Cell*>(this->GetCell(pos));
    }


    for (const auto& ref_cell_pos : cell_ptr->GetReferencedCells()) {
        if (!ref_cell_pos.IsValid()) {

            return;
        }


        if (this->GetCell(ref_cell_pos) == nullptr) {
            auto new_cell = std::make_unique<Cell>(*this);

            CorrectSheetSizeToNewPos(ref_cell_pos);

            sheet_[ref_cell_pos] = std::move(new_cell);

            print_size_ = sheet_size_;

            return;
        }
    }

    CheckCycledCells(*this, cell_ptr);


    for (const auto& cell_pos : cell_ptr->GetReferencedCells()) {
        auto in_cell_ptr = reinterpret_cast<Cell*>(this->GetCell(cell_pos));

        if (in_cell_ptr != nullptr) {
            in_cell_ptr->AddDependingCell(cell_ptr);
        }
    }


    PrepareClearCache(*this, cell_ptr);

    print_size_ = sheet_size_;
}

void Sheet::CheckCycledCells(Sheet& sheet, Cell* init_ptr) {
    
    if (init_ptr == nullptr) {
        return;
    }
    std::stack<Cell*> buffer;
    std::unordered_set<Cell*> closure;

    auto full_stack = [&](Cell* cell_ptr) {
        for (const auto& cell_pos : cell_ptr->GetReferencedCells()) {
            if (!cell_pos.IsValid()) {
                continue;
            }
            auto another_cell = reinterpret_cast<Cell*>(sheet.GetCell(cell_pos));
            if (closure.count(another_cell) == 0) {
                buffer.push(another_cell);
            }
        }
    };

    full_stack(init_ptr);

    while (!buffer.empty()) {
        auto cell_ptr = buffer.top();

        closure.insert(cell_ptr);
        buffer.pop();

        if (init_ptr == cell_ptr) {
            throw CircularDependencyException("cycle link found");
        }

        full_stack(cell_ptr);
    }
}

void Sheet::PrepareClearCache(Sheet& sheet, Cell* cell_ptr) {
    std::stack<Cell*> buffer;
    std::unordered_set<Cell*> closure;

    buffer.push(cell_ptr);

    while (!buffer.empty()) {
        auto cell_ptr = buffer.top();

        closure.insert(cell_ptr);
        buffer.pop();
        cell_ptr->ClearCache();

        for (const auto& cell : cell_ptr->GetDependentCells()) {
            if (closure.count(cell) == 0) {
                buffer.push(cell);
            }
        }
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    if (IsPosOutOfSheet(pos)) {
        return nullptr;
    }

    auto& cell_link = sheet_.at(pos);

    if (cell_link == nullptr || cell_link->IsEmpty()) {
        return nullptr;
    }

    return cell_link.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    if (IsPosOutOfSheet(pos)) {
        return nullptr;
    }

    auto& cell_link = sheet_.at(pos);

    if (cell_link == nullptr || cell_link->IsEmpty()) {
        return nullptr;
    }

    return cell_link.get();
}

void Sheet::ClearCell(Position pos) {

    auto* cell_ptr = reinterpret_cast<Cell*>(GetCell(pos));

    if (cell_ptr == nullptr) {
        return;
    }

    cell_ptr->Clear();

    UpdatePrintableArea();
}

void Sheet::UpdatePrintableArea() {
    int last_non_empty_row = Position::NONE.row;
    int last_non_empty_col = Position::NONE.col;

    for (int row = 0; row < print_size_.rows; ++row) {
        for (int col = 0; col < print_size_.cols; ++col) {
            auto* cell_ptr = reinterpret_cast<Cell*>(GetCell({ row, col }));
            if (cell_ptr != nullptr) {
                if (!cell_ptr->IsEmpty()) {
                    if (row > last_non_empty_row) {
                        last_non_empty_row = row;
                    }

                    if (col > last_non_empty_col) {
                        last_non_empty_col = col;
                    }
                }
            }
        }
    }

    print_size_ = { last_non_empty_row + 1, last_non_empty_col + 1 };
}

Size Sheet::GetPrintableSize() const {
    return print_size_;
}

void Sheet::PrintData(std::ostream& output, DataType data_type) const {
    auto printable_size = GetPrintableSize();

    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            const auto* cell_ptr = reinterpret_cast<const Cell*>(GetCell({ row, col }));
            if (cell_ptr != nullptr) {
                if (data_type == DataType::VALUES) {
                    output << cell_ptr->GetValue();
                }
                if (data_type == DataType::TEXT) {
                    output << cell_ptr->GetText();
                }
            }
            if (col + 1 != printable_size.cols) {
                output << "\t";
            }
        }

        output << "\n";
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintData(output, DataType::VALUES);
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintData(output, DataType::TEXT);
}

bool Sheet::IsPosOutOfSheet(const Position& pos) const {
    return !sheet_.count(pos);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}