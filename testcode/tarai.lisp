(defun tak(x y z) (if (<= x y) y (tak (tak (- x 1) y z) (tak (- y 1) z x) (tak (- z 1) x y))))
(tak 16 8 0)
