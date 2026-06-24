/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QString>

struct DiscoveredDevice
{
    QString ip;
    QString mac;
    bool netburnerDevice = false;
    bool mplcSupported = false;
    QString details;
};
