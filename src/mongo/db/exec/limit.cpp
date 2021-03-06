/**
 *    Copyright (C) 2013 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mongo/db/exec/limit.h"

namespace mongo {

    LimitStage::LimitStage(int limit, WorkingSet* ws, PlanStage* child)
        : _ws(ws), _child(child), _numToReturn(limit) { }

    LimitStage::~LimitStage() { }

    bool LimitStage::isEOF() { return (0 == _numToReturn) || _child->isEOF(); }

    PlanStage::StageState LimitStage::work(WorkingSetID* out) {
        ++_commonStats.works;

        // If we've returned as many results as we're limited to, isEOF will be true.
        if (isEOF()) { return PlanStage::IS_EOF; }

        WorkingSetID id;
        StageState status = _child->work(&id);

        if (PlanStage::ADVANCED == status) {
            *out = id;
            --_numToReturn;
            ++_commonStats.advanced;
            return PlanStage::ADVANCED;
        }
        else {
            if (PlanStage::NEED_FETCH == status) {
                ++_commonStats.needFetch;
            }
            else if (PlanStage::NEED_TIME == status) {
                ++_commonStats.needTime;
            }
            return status;
        }
    }

    void LimitStage::prepareToYield() {
        ++_commonStats.yields;
        _child->prepareToYield();
    }

    void LimitStage::recoverFromYield() {
        ++_commonStats.unyields;
        _child->recoverFromYield();
    }

    void LimitStage::invalidate(const DiskLoc& dl) {
        ++_commonStats.invalidates;
        _child->invalidate(dl);
    }

    PlanStageStats* LimitStage::getStats() {
        _commonStats.isEOF = isEOF();
        auto_ptr<PlanStageStats> ret(new PlanStageStats(_commonStats));
        ret->children.push_back(_child->getStats());
        return ret.release();
    }

}  // namespace mongo
