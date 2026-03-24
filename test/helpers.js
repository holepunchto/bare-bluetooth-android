const os = require('bare-os')

exports.isCI = !!os.getEnv('CI')
