;; unit tests for convert.lsp
(require 'clos-unit)

(load "..\\convert.lsp")

(defclass utest (test-case)
  ())

(def-test-method test_char-list->string ((ob utest))
  (assert-true (equal (char-list->string '(#\a #\b)) "ab"))
)

(def-test-method test_is-empty-line-p ((ob utest))
  (assert-true (is-empty-line-p ""))
  (assert-true (is-empty-line-p "   "))
  (assert-true (is-empty-line-p "  "))
  (assert-true (is-empty-line-p "      "))
)

(defun run-utests ()
  (textui-test-run (get-suite utest)))

(run-utests)

