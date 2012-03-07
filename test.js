var quadprog = require("./build/Release/quadprog");

console.log(
  quadprog.solve(
    [[1,2,3],
     [4,5,6],
     [7,8,9]
    ],
    [1,2,3],

    [[1,2,3],
     [4,5,6],
     [7,8,9]
    ],
    [1,2,3],

    [[1,2,3],
     [4,5,6],
     [7,8,9]
    ],
    [1,2,3]
  )
);