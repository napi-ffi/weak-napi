'use strict';
const assert = require('assert');
const path = require('path');
const { WeakTag, ObjectInfo } = require('node-gyp-build')(path.join(__dirname, '..'));

describe('WeakTag', function() {
  it('throws when given a WeakTag instead of an ObjectInfo', function() {
    let info = new ObjectInfo({}, () => {});
    let wt = new WeakTag(info);
    assert.throws(() => new WeakTag(wt), /First argument needs to be ObjectInfo/);
    // Drop references and collect now, on a live event loop, rather than
    // leaving a WeakTag for process exit to destroy.
    info = null;
    wt = null;
    gc();
  });

  it('throws when given a plain object', function() {
    assert.throws(() => new WeakTag({}), /First argument needs to be ObjectInfo/);
  });

  it('accepts a genuine ObjectInfo', function() {
    let info = new ObjectInfo({}, () => {});
    let wt;
    assert.doesNotThrow(() => { wt = new WeakTag(info); });
    info = null;
    wt = null;
    gc();
  });
});
