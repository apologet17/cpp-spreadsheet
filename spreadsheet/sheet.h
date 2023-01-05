#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class hasher {
public:
    const int ROW_NUM_AS_POW_TWO = 14;
    size_t operator()(const Position& pos) const
    {
        return (pos.col << ROW_NUM_AS_POW_TWO) + (pos.row);
    }
};


enum class DataType {
    VALUES,
    TEXT
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, const std::string& text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CorrectSheetSizeToNewPos(Position pos);

    bool IsPosOutOfSheet(const Position& pos) const;
    void UpdatePrintableArea();
    void PrintData(std::ostream& output, DataType data_type) const;
    void PrepareClearCache(Sheet& sheet, Cell* cell_ptr);
    void CheckCycledCells(Sheet& sheet, Cell* init_ptr);

private:
    Size sheet_size_ = { 0, 0 };
    Size print_size_ = { 0, 0 };
    std::unordered_map<Position, std::unique_ptr<Cell>, hasher> sheet_;
};