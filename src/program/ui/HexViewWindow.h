/*
    Copyright 2015-2024 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBTAS_HEXVIEWWINDOW_H_INCLUDED
#define LIBTAS_HEXVIEWWINDOW_H_INCLUDED

#include "ramsearch/MemSection.h"

#include <QtWidgets/QDialog>
#include <QtWidgets/QComboBox>

#include <vector>

class QHexView;
class IOProcessDevice;

class HexViewWindow : public QDialog {
    Q_OBJECT
public:
    HexViewWindow(QWidget *parent = Q_NULLPTR);

    void start();

    /* Update UI elements */
    void update();

    void seek(uintptr_t addr, int size);

private:
    IOProcessDevice* iodevice;
    QHexView *view;

    QComboBox* sectionChoice;

    /* Array of all memory sections parsed from /proc/self/maps */
    std::vector<MemSection> memsections;

public slots:
    void update_layout();
    void update_sections();
    void switch_section();

};

#endif
