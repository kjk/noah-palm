;(load         "c:\\kjk\\src\\mine\\noah_palm\\converter\\convert.lsp")
;(load         "c:\\kjk\\src\\mine\\noah_palm\\converter\\convert.fas")
;(compile-file "c:\\kjk\\src\\mine\\noah_palm\\converter\\convert.lsp")
;(time (progn (make-engpol-words-hash) nil))

(in-package :common-lisp-user)

(defparameter *file-sets* 'win)
;(defparameter *file-sets* 'unix)

(if (eq *file-sets* 'unix)
  (progn
    (defconstant *wordnet-dir* "/home/kjk/src/dict_data/wordnet/dict/")
    (defconstant *eng-pol-file-name* "/home/kjk/src/dict_data/engpol/eng_pol.txt")
    (defconstant *eng-pol-words-cache-file-name* "/home/kjk/src/dict_data/engpol/eng_pol_words.txt")
    ))

(if (eq *file-sets* 'win)
  (progn
    (defconstant *wordnet-dir*       "c:\\kjk\\src\\mine\\noah_dicts\\wordnet16\\")
    (defconstant *eng-pol-file-name* "c:\\kjk\\src\\mine\\noah_dicts\\eng_pol.txt")
    (defconstant *eng-pol-words-cache-file-name* "c:\\kjk\\src\\mine\\noah_dicts\\eng_pol_words.txt")
    ))

(defun file-exists-p (path) (probe-file path))

(defun percent (original packed)
  (float (* 100 (/ (- original packed) original))))

;; misc functions
(defun make-const-len-string (len str-to-copy init-el)
  "make a string that has constant len, copies up to len chars
from str-to-copy and the rest of string is filled with init-el"
  (let ((str (make-string len :initial-element init-el))
	(str-to-copy-len (min (length str-to-copy) len)))
    (dotimes (n str-to-copy-len)
	     (setf (aref str n) (aref str-to-copy n)))
    str))

(defun int->char (int)
  "just another name for doing number->char conversion"
  (code-char int))

(defun int->string (num)
  (make-string 1 :initial-element (int->char num)))

(defun int-list->string (int-list)
  (map 'string #'int->char int-list))

(defun char->int (char)
  (char-code char))

(defun convert-string-to-integer (str &optional (radix 10)) 
  "Given a digit string and optional radix, return an integer." 
  (do ((j 0 (+ j 1)) 
       (n 0 (+ (* n radix) 
               (or (digit-char-p (char str j) radix) 
                   (error "Bad radix-~D digit: ~C" 
                          radix 
                          (char str j)))))) 
      ((= j (length str)) n)))
  
(defun dec-string->integer (string)
  (convert-string-to-integer string))

(defun hex-string->integer (string)
  (convert-string-to-integer string 16))

; taken from "On Lisp" Graham's book
(defun mkstr (&rest args)
  (with-output-to-string (s) (dolist (a args) (princ a s))))

; much faster than the above (around 24 times) not very lispy, though
(defun string-list->string (list)
  (let ((total-len 0))
    (mapc #'(lambda (x) (incf total-len (length x))) list)
    (let ((result-str (make-string total-len))
	  (i 0))
      (dolist (el list)
	      (dotimes (n (length el))
		       (progn
			 (setf (aref result-str i) (aref el n))
			 (incf i))))
      result-str)))

(defun set-bit-on-integer (num bit)
  (boole boole-ior num (ash 1 bit)))

(defun split-string (str chars &rest opts)
  "Split the string on chars."
  (declare (string str) (sequence chars))
  (apply #'split-seq str (lambda (ch) (declare (character ch)) (find ch chars))
         opts))

(defun split-seq (seq pred &key (start 0) end key strict)
  "Return a list of subseq's of SEQ, split on predicate PRED.
Start from START, end with END.  If STRICT is non-nil, collect
zero-length subsequences too.
  (split-seq SEQ PRED &key (start 0) end key strict)"
  (declare (sequence seq) (type (function (t t) t) pred) (fixnum start))
  (loop :for st0 = (if strict start
                       (position-if-not pred seq :start start
                                        :end end :key key))
        :then (if strict (if st1 (1+ st1))
                  (position-if-not pred seq :start (or st1 st0)
                                   :end end :key key))
        :with st1 = 0 :while (and st0 st1) :do
        (setq st1 (position-if pred seq :start st0 :end end :key key))
        :collect (subseq seq st0 st1)))

(defun string-compress (prev-str next-str)
  "very simple differential compressor, given two strings returns
string that consists of: first byte (<32) is the number of chars
those strings have the same in the beginning and the rest is just
those chars that are not common"
  (let* ((in-common (min (mismatch prev-str next-str) 31))
	 (c-str (make-string (1+ (- (length next-str) in-common))
			     :initial-element (int->char in-common))))
    (setf (subseq c-str 1) (subseq next-str in-common))
    c-str))

;; functions for binary i/o
(defun int->bytes (num no-bytes)
  "split an integer num into separate bytes, eg.
258 to (1,2) (=1*256+2)" 
 (do ((rest (mod num 256) (mod num 256))
       (num-list '() (push rest num-list))
       (n no-bytes (- n 1)))
      ((= n 0) num-list)
      (if (> num 0)
	  (setq num (values (truncate (/ num 256)))))))

(defun int->multibyte-string (num no-bytes)
  (let ((str-out (make-string no-bytes))
	(n -1))
    (dolist (el (int->bytes num no-bytes))
	    (setf (aref str-out (incf n)) (int->char el)))
    str-out))

(defun int24->string (num)
  (int->multibyte-string num 3))

(defun int->multibyte-string2 (num no-bytes)
  (map 'string #'int->char (int->bytes num no-bytes)))

(defun write-multibyte-int (num no-bytes stream)
  "write an int as n-byte integer in big-endian (motorola)
format to an output stream"
  (mapc #'(lambda (x) (write-byte x stream))
	  (int->bytes num no-bytes)))

(defun write-int32 (num stream)
  (write-multibyte-int num 4 stream))

(defun write-int24 (num stream)
  (write-multibyte-int num 3 stream))

(defun write-int16 (num stream)
  (write-multibyte-int num 2 stream))

(defun write-int8 (num stream)
  (write-byte num stream))

(defun write-string-as-bytes (s ostream)
  (map nil #'(lambda (c) (write-byte (char-code c) ostream)) s))

;; functions that operate on PDB structure
(defconstant +days-in-a-month+ '(31 29 31 30 31 30 31 31 30 31 30 31))
(defun days-from-begin-year (month day)
  (let ((res day))
    (do ((days +days-in-a-month+ (cdr days))
	 (n 0 (+ n 1)))
	((= n month) res)
	(setq res (+ res (car days))))))
	
;FIXME: broken as hell
(defun secs-from-jan (year month day hour min sec)
  (+ sec
     (* 60 (+ min
	      (* 60 (+ hour
		       (* 24 (+ (days-from-begin-year month day)
				(* 365 (- year 1904))))))))))

;(secs-from-jan 2000 02 20 12 33 00)

(defstruct freq-el
  i j freq)

; given the frequency of every character pair,
; find out max n non-conflicting pairs of greatest frequency
(defun freq-sort (freq-array n)
  (let ((list '()))
    (dotimes (i 256)
	     (dotimes (j 256)
		      (let ((freq (aref freq-array i j)))
			(if (not (= 0 freq))
			    (push (make-freq-el :i i :j j :freq freq) list)))))
    (setq list (sort list #'(lambda (el1 el2) (> (freq-el-freq el1) (freq-el-freq el2)))))
;    (format t "~& we have ~D sorted freq elements~%" (list-length list))
    (let ((out-list '()))
      (loop while (and
		   (< (list-length out-list) n)
		   (not (null list)))
	    do (if (not (freq-conflicts-p out-list (car list)))
		   (push (car list) out-list))
	    (setq list (cdr list)))
      (reverse out-list))))

(defun freq-el-conflicts-p (freq-el1 freq-el2)
  (or (= (freq-el-i freq-el1) (freq-el-j freq-el2))
      (= (freq-el-j freq-el1) (freq-el-i freq-el2))))

(defun freq-conflicts-p (freq-list freq-el)
  "the freq-el conflicts with freq-list it's i=j or j=i for any element on the list"
  (do ((list-rest freq-list (cdr list-rest)))
      ((or
	(null list-rest)
	(freq-el-conflicts-p freq-el (car list-rest))) (if (null list-rest)
							   nil
							 t))))
; packing:
; add all strings that will be packed
; build data used for compression
; compress all individual strings
; to compress we need a list of strings and their corresponding codes
(defstruct pack-info
  (str-list '())
  (str->code (make-array '(256) :element-type 'string))
  (freq (make-array '(256 256) :element-type 'integer :initial-element 0))
  (compression-table (make-array '(256 256) :element-type 'integer :initial-element 0))
  ; this maps a code to a character or a pair of characters
  (code->char (make-array '(256 2) :element-type 'integer :initial-element 0))
  ; this lets us map characters or pair of characters to codes
  (char->code (make-array  '(256) :element-type 'integer :initial-element 0))
  (used-codes 1) ; zero always maps to zero
  (one-char-codes 0))

; make n first codes unavailable
(defun pack-info-reserve-codes (pack-info n)
  (dotimes (i n)
      (setf (aref (pack-info-char->code pack-info) i) i)
      (setf (aref (pack-info-code->char pack-info) i 0) i))
  (setf (pack-info-used-codes pack-info) n))

(defun build-freq-for-string (pack-info str)
  (let* ((list (map 'list #'char->int str))
	(el1 (car list))
	(list (cdr list))
	(freq (pack-info-freq pack-info)))
    (dolist (el2 list)
	    (incf (aref freq el1 el2))
	    (setq el1 el2))))

(defun pi-char->code (pack-info char-as-int)
  "map a given character to a code, create new mapping if doesn't exist yet"
  (let* ((char->code (pack-info-char->code pack-info))
	 (code (aref char->code char-as-int)))
    (if (and
	 (= 0 code)
	 (not (= 0 char-as-int)))
	(let ((code->char (pack-info-code->char pack-info))
	      (used-codes (pack-info-used-codes pack-info)))
;	  (format t "~& coding ~D (~A) char as code ~D ~%" char-as-int
;		  (int->string char-as-int) used-codes)
	  (setf (aref char->code char-as-int) used-codes)
	  (setf (aref code->char used-codes 0) char-as-int)
	  (incf (pack-info-used-codes pack-info))
	  (setq code used-codes)))
    code))

(defun calc-new-codes-count (used-codes)
  (if (> used-codes 256) (error "used-codes must be smaller that 256"))
  (let ((max (- 256 used-codes))
	(list '((200 50) (120 40) (60 25) (30 10) (-1 18))))
    (do ((list-rest list (rest list-rest)))
	((> max (car (car list-rest)))
	 (if (= -1 (car (car list-rest))) max (cadr (car list-rest)))))))

(defun pack-info-add-pair (pack-info x y)
  (let ((used (pack-info-used-codes pack-info)))
    (setf (aref (pack-info-compression-table pack-info) x y) used)
    (setf (aref (pack-info-code->char pack-info) used 0) x)
    (setf (aref (pack-info-code->char pack-info) used 1) y)
    (incf (pack-info-used-codes pack-info))))

(defun chars-from-code-rec (pack-info code res)
  (let* ((code->char (pack-info-code->char pack-info))
	(one-char-codes (pack-info-one-char-codes pack-info))
	(char (aref code->char code 0)))
    (if (<= char one-char-codes)
	(push char res)
      (progn
	(push (chars-from-code-rec pack-info char res) res)
	(push (chars-from-code-rec pack-info (aref code->char code 1) res) res)))))

(defun build-string-from-code (pack-info code)
  (let ((out-char-list-as-ints (reverse (chars-from-code-rec pack-info code '()))))
    (int-list->string out-char-list-as-ints))) 

(defun pack-info-build-pack-strings (pack-info)
  "after all code data has been built generate strings for compression"
  (let ((out-list '())
	(str->code (pack-info-str->code pack-info))
	(n -1))
    (dotimes (code 256)
	     (push (tmp-unpack-string pack-info (int->string code)) out-list))
    (dolist (el (sort out-list #'(lambda (str1 str2) (< (length str1) (length str2)))))
	    (setf (aref str->code (incf n)) el))))
	    
(defun pack-info-build-pack-data (pack-info)
  "having build code table and initial frequency data,
build the actual data to do compression"
  (setf (pack-info-one-char-codes pack-info) (pack-info-used-codes pack-info))
  (loop
   while (< (pack-info-used-codes pack-info) 256)
   do (pack-info-build-pack-data-one-pass
       pack-info
       (calc-new-codes-count (pack-info-used-codes pack-info))))
  (pack-info-build-pack-strings pack-info))

(defun pack-info-clear-compression-table (pack-info)
  (let ((compression-table (pack-info-compression-table pack-info)))
    (dotimes (i 256)
	     (dotimes (j 256)
		      (setf (aref compression-table i j) 0)))))

(defun pack-info-build-pack-data-one-pass (pack-info codes-to-use)
  (let* ((sorted-freq (freq-sort (pack-info-freq pack-info) codes-to-use))
	 (real-new-codes (list-length sorted-freq))
	 (new-list '()))
    (format t "~& used-codes=~D ~%" (pack-info-used-codes pack-info))
    (format t "~& real-new-codes=~D ~%" real-new-codes)
    (if (< real-new-codes 1) (error "new-codes must be greater that 0"))
    (dolist (el sorted-freq)
	    (pack-info-add-pair pack-info (freq-el-i el) (freq-el-j el)))
    (dolist (str (pack-info-str-list pack-info))
	    (push (tmp-pack-string pack-info str) new-list))
    (setf (pack-info-str-list pack-info) new-list)
    (pack-info-clear-compression-table pack-info)
    (if (< (pack-info-used-codes pack-info) 256)
	(pack-info-build-freq-info pack-info))))

(defun pack-info-build-freq-info (pack-info)
  (dotimes (i 256)
	   (dotimes (j 256)
		    (setf (aref (pack-info-freq pack-info) i j) 0)))
  (dolist (str (pack-info-str-list pack-info))
	  (build-freq-for-string pack-info str)))

(defun tmp-unpack-string (pack-info str)
  (let ((list (map 'list #'char->int str)))
    (tmp-unpack-str-rec pack-info list (tmp-unpack-str-need-another-pass-p
					(pack-info-one-char-codes pack-info) list))))

(defun tmp-unpack-str-rec (pack-info list need-another-pass)
  (if need-another-pass
      (let ((res '()))
	(dolist (el list)
		(if (< el (pack-info-one-char-codes pack-info))
		    (push el res)
		  (progn
		    (push (aref (pack-info-code->char pack-info) el 0) res)
		    (push (aref (pack-info-code->char pack-info) el 1) res))))
	(tmp-unpack-str-rec pack-info (reverse res) (tmp-unpack-str-need-another-pass-p
					   (pack-info-one-char-codes pack-info) res)))
    (map 'string #'(lambda (x) (int->char (aref (pack-info-code->char pack-info) x 0)))
	  list)))

(defun tmp-unpack-str-need-another-pass-p (one-char-codes list)
  (do ((rest list (cdr rest)))
      ((or
	(null rest)
	(>= (car rest) one-char-codes))
       (if (null rest)
	   nil
	 t))))

(defun tmp-pack-string (pack-info str)
  (let ((list (map 'list #'char->int str)))
    (tmp-pack-str-rec (pack-info-compression-table pack-info) list '())))
		     
(defun tmp-pack-str-rec (compression-table str-rest-as-list result)
  (if (> (list-length str-rest-as-list) 1)
      (let* ((x (first str-rest-as-list))
	     (y (second str-rest-as-list))
	     (code (aref compression-table x y)))
	(if (= 0 code)
	    (tmp-pack-str-rec compression-table (cdr str-rest-as-list) (cons x result))
	  (tmp-pack-str-rec compression-table (cddr str-rest-as-list) (cons code result))))
    (if (= 0 (list-length str-rest-as-list))
	(int-list->string (reverse result))
      (int-list->string (reverse (cons (car str-rest-as-list) result))))))

; given a string return string that is built by
; replacing chars with codes. 
(defun code-string (pack-info str)
  (let* ((str-len (length str))
	 (str-out (make-string str-len))
	 (char-list (map 'list #'char->int str))
	 (index -1))
    (dolist (char char-list)
	    (setf (aref str-out (incf index)) (int->char (pi-char->code pack-info char))))
    str-out))

; given a string return string that is build by
; replacing chars with codes. 
; (defun code-string2 (pack-info str)
;   (let* ((code-list (map 'list #'(lambda (x) (pi-char->code pack-info (char->int x))) str)))
;     (int-list->string code-list)))

; given a coded string return string that is built by
; replacing codes with chars
(defun decode-string (pack-info str)
  (let* ((str-len (length str))
	 (str-out (make-string str-len))
	 (char-list (map 'list #'char->int str))
	 (code->char (pack-info-code->char pack-info))
	 (index -1))
    (dolist (char char-list)
	    (setf (aref str-out (incf index)) (int->char (aref code->char char 0))))
    str-out))

(defun decode-nth-string (pack-info n)
  (decode-string pack-info (nth n (pack-info-str-list pack-info))))


(defun pack-info-add-string (pack-info str)
  (let ((coded-string (code-string pack-info str)))
    (build-freq-for-string pack-info coded-string)
    (push coded-string (pack-info-str-list pack-info))))

(defun pack-info-add-compressed-word (pack-info str)
  (pack-info-add-string pack-info (subseq str 1)))

; given a pack-info, string, offset within a string, find a code
; that best matches this string
(defun get-pack-code (pack-info str offset)
  (let ((str->code (pack-info-str->code pack-info))
	(max-len (- (length str) offset)))
    (do* ((i 255 (1- i))
	  (compr-str (aref str->code i) (aref str->code i))
	  (compr-str-len (length compr-str)
			 (length compr-str)))
	 ((and
	   (<= compr-str-len max-len)
	   (string= str compr-str :start1 offset :end1 (+ offset compr-str-len)))
	  i))))

; (defun get-pack-code2 (pack-info str offset)
;   (let ((hash-table (pack-info-hash-table pack-info))
; 	(max-len (- (length str) offset)))
;     (do* ((str-len max-len (1- str-len))
; 	  (res (gethash (subseq str offset str-len) hash-table)
; 	       (gethash (subseq str offset str-len) hash-table)))
; 	 (if res
; 	     (return res)))
;     (error "this should never happen")))

(defun str-by-pack-code (pack-info code)
  (aref (pack-info-str->code pack-info) code))

(defun pack-string (pack-info str)
  (let ((str-len (length str))
	(pack-str "")
	(pack-code 0))
    (do ((offset 0 (+ offset (length pack-str)))
	 (res-str-list '() (push (int->string pack-code) res-str-list)))
	((>= offset str-len) (string-list->string (reverse res-str-list)))
	(setf pack-code (get-pack-code pack-info str offset))
	(setf pack-str (str-by-pack-code pack-info pack-code)))))

(defun unpack-string (pack-info str)
   (let ((char-list (map 'list #'char->int str))
	 (out-str '()))
     (dolist (el char-list)
	     (push (str-by-pack-code pack-info el) out-str))
     (string-list->string (reverse out-str))))

(defun unpack-string2 (pack-info str)
  (string-list->string (mapcar
			     #'(lambda (code) (str-by-pack-code pack-info code))
			     (map 'list #'char->int str))))

(defun make-compression-data-record (pack-info)
  (let* ((rec (make-instance 'pdb-record))
	 (str->code (pack-info-str->code pack-info))
	 (max-compr-str-len (length (aref str->code 255)))
	 (count-of-this-len 0)
	 (curr-len (length (aref str->code 0))))
    (pdb-record-add-int16 rec max-compr-str-len)
    (dotimes (n 256)
	     (if (= curr-len (length (aref str->code n)))
		 (incf count-of-this-len)
	       (progn
		 (pdb-record-add-int16 rec count-of-this-len)
		 (dotimes (i (- (length (aref str->code n)) curr-len 1))
			  (pdb-record-add-int16 rec 0))
		 (setq curr-len (length (aref str->code n)))
		 (setq count-of-this-len 1))))
    (pdb-record-add-int16 rec count-of-this-len)
    (dotimes (n 256)
	     (pdb-record-add-string rec (aref str->code n)))
    rec))

(defconstant +max-record-len+ 63900)
(defconstant +max-words-in-cache+ 500)
(defconstant +min-words-in-cache+ 300)

(defstruct pdb
  name
  (file-attributes 0)
  (version 0)
  creation-date modification-date
  (last-backup-date 0)
  (modification-number 0)
  (app-info-area 0)
  (sort-info-area 0)
  db-type creator-id
  (record-list '()))

(defclass pdb-record ()
  (
   (attributes :initform 0)
   (str-list :initform '())
   (size :initform 0)
   )
  )

(defclass pdb-record-list ()
  ((recs :initform '())
   (max-size :initform 64000)
   (new-record-hook :initform '())))

(defmethod pdb-record-list-get-records ((rec-list pdb-record-list))
  (reverse (slot-value rec-list 'recs)))

(defmethod pdb-record-list-new-record ((rec-list pdb-record-list))
  (let ((new-rec-hook (slot-value rec-list 'new-record-hook)))
    (push (make-instance 'pdb-record) (slot-value rec-list 'recs))
    (if (not (null new-rec-hook))
	(funcall new-rec-hook (length (slot-value rec-list 'recs))))))

(defmethod current-record-size ((rec-list pdb-record-list))
  (let ((cur-record (car (slot-value rec-list 'recs))))
    (if (not (null cur-record))
	(pdb-record-size cur-record)
      0)))

(defmethod pdb-record-add-string ((rec-list pdb-record-list) str)
  (let* ((cur-rec (car (slot-value rec-list 'recs))))
    (if (or (null cur-rec)
	    (> (+ (length str) (current-record-size rec-list))
	       (slot-value rec-list 'max-size)))
	(pdb-record-list-new-record rec-list))
    (pdb-record-add-string (car (slot-value rec-list 'recs)) str)))

(defmethod pdb-record-add-string ((record pdb-record) str)
  (push str (slot-value record 'str-list))
  (setf (slot-value record 'size) (+ (slot-value record 'size) (length str))))

(defmethod pdb-record-add-int32 (record num)
  (pdb-record-add-string record (int->multibyte-string num 4)))

(defmethod pdb-record-add-int24 (record num)
  (pdb-record-add-string record (int->multibyte-string num 3)))

(defmethod pdb-record-add-int16 (record num)
  (pdb-record-add-string record (int->multibyte-string num 2)))

(defmethod pdb-record-add-int8 (record num)
  (pdb-record-add-string record (int->string num)))

(defmethod pdb-record-add-universal (record list)
  (dolist (el list)
	  (if (consp el)
	      (case (first el)
		    ((int8) (pdb-record-add-int8 record (second el)))
		    ((int16) (pdb-record-add-int16 record (second el)))
		    ((int24) (pdb-record-add-int24 record (second el)))
		    ((int32) (pdb-record-add-int32 record (second el))))
	    (if (stringp el)
		(pdb-record-add-string record el)))))

(defmethod pdb-record-size ((record pdb-record))
  (if (> (slot-value record 'size) 65530)
      (error "record too big"))
  (slot-value record 'size))

(defmethod pdb-record-size ((recs pdb-record-list))
  (let ((size 0))
    (dolist (rec (slot-value recs 'recs))
      (setq size (+ size (slot-value rec 'size))))
    size))

(defmethod write-record ((record pdb-record) stream)
  (dolist (str (reverse (slot-value record 'str-list)))
	  (write-string-as-bytes str stream)))

(defun pdb-set-dates (pdb)
  "set creation,modification,last backup dates on a pdb"
;  (let ((current-time (secs-from-jan 2000 02 20 12 33 00)))
  (let ((current-time #x33b8bda5))
    (setf (pdb-creation-date pdb) current-time)
    (setf (pdb-modification-date pdb) current-time)
    (setf (pdb-last-backup-date pdb) current-time)))

(defun pdb-add-record (pdb record)
  "add record to a pdb"
  (if (null record)
      (error "null record in pdb-add-record"))
  (push record (pdb-record-list pdb)))

(defun write-pdb-header (pdb stream)
  (write-string-as-bytes (make-const-len-string 32 (pdb-name pdb) (int->char 0)) stream)
  (write-int16  (pdb-file-attributes pdb) stream)
  (write-int16  (pdb-version pdb) stream)
  (write-int32  (pdb-creation-date pdb) stream)
  (write-int32  (pdb-modification-date pdb) stream)
  (write-int32  (pdb-last-backup-date pdb) stream)
  (write-int32  (pdb-modification-number pdb) stream)
  (write-int32  (pdb-app-info-area pdb) stream)
  (write-int32  (pdb-sort-info-area pdb) stream)
  (write-string-as-bytes (pdb-db-type pdb) stream)
  (write-string-as-bytes (pdb-creator-id pdb) stream)
  (write-int32  0 stream) ; seed-id is always zero
  (write-int32  0 stream) ; next-record-list is always zero
  (format t "~% number of records: ~D~&" (length (pdb-record-list pdb)))
  (write-int16  (length (pdb-record-list pdb)) stream))

;;(defun write-pdb-record (filename record)
;;   (with-open-file
;;    (ostream filename :direction :output :if-exists :supersede :element-type 'unsigned-byte)
;;    (write-record record ostream)))

(defun write-pdb (pdb-name pdb)
  (with-open-file
   (ostream pdb-name :direction :output :if-exists :supersede :element-type 'unsigned-byte)
   (write-pdb-header pdb ostream)
  ;; for every pdb record we need to write record header,
  ;; which is record offset in the file + record attributes
  ;; + 3 other bytes that should be 0
   (let* ((rec-count (length (pdb-record-list pdb)))
	  (rec-num 0)
	  (rec-offset (+ 78 (* rec-count 8))))
     (dolist (record (remove '() (reverse (pdb-record-list pdb))))
	     (write-int32 rec-offset ostream)
	     (write-int8 (slot-value record 'attributes) ostream)
	     (write-int8 0 ostream)
	     (write-int8 0 ostream)
	     (write-int8 0 ostream)
	     (if (< (pdb-record-size record) 0)
		 (error "record size negative"))
	     (setq rec-num (+ 1 rec-num))
	     (setq rec-offset (+ rec-offset (pdb-record-size record)))))
   (dolist (record (remove '() (reverse (pdb-record-list pdb))))
	   (write-record record ostream))))

; creates an iterator that returns words in a sequence
; this one is based on a simple list
(defmacro make-list-iterator-with-function (list fun)
  `(let ((rest ,list)
	 (orig-list ,list))
     #'(lambda (action)
	 (cond ((eql action 'reset) (setq rest orig-list))
	       ((eql action 'get-next)
		(if (null rest)
		    nil
		  (progn
		    (let ((list-el (car rest)))
		      (setq rest (cdr rest))
		      (,fun list-el)))))
	       ((eql action 'get-first)
		(if (null rest)
		    nil
		  (progn
		    (let ((list-el (car orig-list)))
		      (setq rest (cdr orig-list))
		      (,fun list-el)))))
	       (t (error "unknown command"))))))

(defun make-list-iterator (list)
  (if (null list)
      (error "empty list in make-list-iterator"))
  (make-list-iterator-with-function list identity))

(defmacro make-new-record (rec-list)
  `(car (push (make-instance 'pdb-record) ,rec-list)))

;; word-packer produces pdb records describing compressed
;; words given the function that iterates over words

(defstruct word-packer
  (words-records '())
  (word-cache-info '())
  (word-cache-record  '())
  (words-in-cache 0)
  (current-word 0)
  (total-words-len 0)
  (total-compressed-words-len 0)
  (max-word-len 0)
  (words-count 0)
  (longest-word "")
  (word-compressor (make-word-compressor))
  (current-offset-in-word-record 0)
  (pack-info (make-pack-info)))

(defmethod word-packer-dump-info ((packer word-packer))
    (format t "~&Size of word cache info:~D~%"
	    (* 6 (length (word-packer-word-cache-info packer))))
    (format t "~&Total number of words :~D~%"
	    (word-packer-words-count packer))
    (format t "~&Total words len      :~D~%"
	    (word-packer-total-words-len packer))
    (format t "~&Total compr words len:~D~%"
	    (word-packer-total-compressed-words-len packer))
    (format t "~&Longest word has len          : ~D~%"
	    (word-packer-max-word-len packer))
    (format t "~&Compression ratio for words:~D~%"
	    (percent
	     (word-packer-total-words-len packer)
	     (word-packer-total-compressed-words-len packer))))

(defmethod packer-add-word-cache-entry ((packer word-packer) word)
  (push (make-word-cache-info :word-number
			      (word-packer-current-word packer)
			      :record-no
			      (- (length (word-packer-words-records packer)) 1)
			      :record-offset
			      (word-packer-current-offset-in-word-record packer))
	(word-packer-word-cache-info packer))
  (setf (word-packer-words-in-cache packer) 0)
  (format t "~&Cache entry on word no ~D, record no ~D, offset ~D: ~A~%"
	  (word-packer-current-word packer)
	  (- (length (word-packer-words-records packer)) 1)
	  (word-packer-current-offset-in-word-record packer)
	  word))

(defmethod get-word-cache-info-pdb-record ((packer word-packer))
  (let ((record (word-packer-word-cache-record packer)))
    (if (null record)
	(setf (word-packer-word-cache-record packer)
	      (make-word-cache-record (reverse
				       (word-packer-word-cache-info packer))))
      record)))

(defmethod packer-make-words-record ((packer word-packer))
  (setf (word-packer-current-offset-in-word-record packer) 0)
  (funcall (word-packer-word-compressor packer) 'reset)
  (make-new-record (word-packer-words-records packer)))
  
(defmethod word-packer-build-word-pdb-data ((packer word-packer) word-iterator)
  (let ((word-compressor (word-packer-word-compressor packer))
	(pack-info (word-packer-pack-info packer))
	(force-new-word-cache-p 't)
	(word-len 0)
	(compressed-word "")
	(rec (packer-make-words-record packer)))
    ;; create compression data for words
    (pack-info-reserve-codes pack-info 32)
    (do ((word (funcall word-iterator 'get-first) (funcall word-iterator 'get-next)))
	((null word))
      (pack-info-add-compressed-word pack-info
				     (funcall word-compressor 'compress word)))
    (format t "~& building pack data for words ~%")
    (pack-info-build-pack-data pack-info)
    (format t "~& compressing words ~%")
    (funcall word-iterator 'reset)
    ;; go through every word, pack it, put into word record, build word cache
    ;; on the way
    (setf (word-packer-words-count packer) 0)
    (do ((word (funcall word-iterator 'get-first) (funcall word-iterator 'get-next)))
	((null word))
      (setq word-len (length word))
      (incf (word-packer-words-count packer))
      (if (> (+ word-len (pdb-record-size rec)) +max-record-len+)
	  (progn
	    (setq rec (packer-make-words-record packer))
	    (setq force-new-word-cache-p 't)))
      (if (> (word-packer-words-in-cache packer) +max-words-in-cache+)
	  (progn
	    (funcall word-compressor 'reset)
	    (setq force-new-word-cache-p 't)))
      (setq compressed-word
	    (pack-string pack-info (funcall word-compressor 'compress word)))
      (if (and (= 0 (funcall word-compressor 'last-in-common))
	       (> (word-packer-words-in-cache packer) +min-words-in-cache+))
	  (setq force-new-word-cache-p 't))
      (if force-new-word-cache-p
	  (progn
	    (packer-add-word-cache-entry packer word)
	    (setq force-new-word-cache-p nil)))
      (pdb-record-add-string rec compressed-word)
      (incf (word-packer-total-words-len packer) word-len)
      (incf (word-packer-current-word packer))
      (incf (word-packer-words-in-cache packer))
      (incf (word-packer-total-compressed-words-len packer) (length compressed-word))
      (incf (word-packer-current-offset-in-word-record packer) (length compressed-word))
      (if (> word-len (word-packer-max-word-len packer))
	  (progn
	    (setf (word-packer-max-word-len packer) word-len)
	    (setf (word-packer-longest-word packer) word))))
    ;; Hack to add 0 at the end so that uncompressor won't trip on the last word
    (setq compressed-word
	  (pack-string pack-info (funcall word-compressor 'compress "rare")))
    ;;(format "~& ::~A~%" compressed-word)
    (pdb-record-add-string rec compressed-word)))
;    (pdb-record-add-string rec
;			   (pack-string pack-info
;					(make-string 1 :initial-element
;						     (int->char 0))))))
;(defun test-word-packer ()
;  (let ((word-packer (make-word-packer))
;	(word-iterator (make-list-iterator '("only" "the" "lonely"))))
;    (word-packer-build-word-pdb-data word-packer word-iterator)))

(defstruct word-cache-info
  word-number
  record-no
  record-offset)

;; given a list of word-cache-info structs, will make a pdb record
;; out of them
(defun make-word-cache-record (word-cache-info)
  "make a record that contains info to let us faster find a word, for every
word that is not compressed it contains its number, record number and offset
in a record"
  (let ((record (make-instance 'pdb-record)))
;;;     (format t "~& Cache entries: ~D ~%" (length *word-cache-info-list*))
    (dolist (el word-cache-info)
      (pdb-record-add-int32 record (word-cache-info-word-number el))
      (pdb-record-add-int16 record (word-cache-info-record-no el))
      (pdb-record-add-int16 record (word-cache-info-record-offset el)))
    record))

(defstruct word-compressor-state
  (prev-word "")
  (last-common 0))

(defmethod reset-compressor ((compressor word-compressor-state))
  (setf (word-compressor-state-prev-word compressor) "")
  (setf (word-compressor-state-last-common compressor) 0))

(defmethod compress-word ((compressor word-compressor-state) (word string))
  (let* ((prev-word (word-compressor-state-prev-word compressor))
	 (in-common (mismatch prev-word word))
	 (compressed-word (string-compress prev-word  word)))
    (setf (word-compressor-state-prev-word compressor) word)
    (setf (word-compressor-state-last-common compressor) in-common)
;     (if (equal "" (word-compressor-state-prev-word compressor))
; 	(format t "~& empty prev word detected for words ~A ~A~%"
; 		prev-word word))
;     (if (= 0 in-common)
; 	(Format T "~& zero in common for: ~A and ~A~%" prev-word word))
    compressed-word))

(defun make-word-compressor ()
  (let ((prev-word "")
	(last-common 0))
    #'(lambda (action &optional word)
	(cond ((eql action 'reset)
	       (setq prev-word "")
	       (setq last-common 0))
	      ((eql action 'compress)
	       (let ((compressed-word (string-compress prev-word word)))
		 (setq last-common (mismatch prev-word word))
		 (setq prev-word word)
		 compressed-word))
	      ((eql action 'last-in-common)
	       last-common)
	      (t (error "unknown action in make-word-compressor"))))))


(defun char-list->string (char-list)
  (map 'string
      #'(lambda (x) x)
      char-list))


(defun whitespacep (c)
    (case c
        ;;((#\Space #\Newline #\Linefeed #\Tab #\Return #\Page) t)
        ((#\Space #\Newline #\Tab #\Return #\Page) t)
        (t nil)))

(defconstant *ws-list* '(#\Space #\Newline #\Linefeed #\Tab #\Return #\Page))
(defconstant new-line-s (char-list->string '(#\newline)))

;; empty if number of non-whitespaces is zero (i.e. all of them are whitespaces)
(defun is-empty-line-p (str)
    (= 0 (count-if-not #'whitespacep str)))

(defun word-char-p (c) (position c "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.'-"))

(defun word-p (str)
    "return str if it matches a word or NIL otherwise"
    (cond
        ((= 0 (length str)) nil)
        ((string= str (remove-if #'(lambda (c) (not (word-char-p c))) str)) str)
        (t nil)))

(defun write-hash-keys-to-file (hash file)
    (with-open-file
        (out-stream file :direction :output)
        (maphash
           #'(lambda (key val)
                (declare (ignore val))
                (format out-stream "~A~%" key)) hash)))

(defun make-words-hash-from-file (file)
  (let ((hash (make-hash-table :test 'equal :size 50000)))
    (with-open-file
     (in-stream file :direction :input)
     (do ((l (read-line in-stream) (read-line in-stream nil 'eof)))
        ((eq l 'eof) hash)
        (if (word-p l)
            (setf (gethash l hash) l))))))

(defun make-words-hash-from-file-with-cache (file cache-file)
    (if (file-exists-p cache-file)
        (make-words-hash-from-file cache-file)
        (let ((h (make-words-hash-from-file file)))
            (write-hash-keys-to-file h cache-file)
            h)))

;; construct a hash table that has words from english-polish dict
(defun make-engpol-words-hash ()
    (make-words-hash-from-file-with-cache *eng-pol-file-name* *eng-pol-words-cache-file-name*))

;;(defun make-special-words-hash ()
;;  (let ((hash (make-hash-table :test 'equal :size 50000))
;;	(file-name "/home/kjk/src/noah_pro/converter/noah_deletes.txt")
;;	(word-regexp (excl:compile-regexp "^\\([ 0-9a-zA-Z\.'-]+\\)$"))
;;	(word ""))
;;    (with-open-file
;;     (in-stream file-name :direction :input)
;;     (do ((l (read-line in-stream) (read-line in-stream nil 'eof)))
;;	 ((eq l 'eof) hash)
;;	 (if (match-word l))
;;	     (setf (gethash word hash) word))))
;;	 (if (multiple-value-setq (match whole word) (excl:match-regexp word-regexp l))
;;	     (setf (gethash word hash) word))))
;;    hash))

(defstruct wn-lemma
  lemma (id 0) (list-of-synsets '()))

(defstruct wn-gloss
  list-of-defs
  list-of-examples)

(defstruct wn-synset
  (id 0)
  lex-filenum ; adj.all, noun.Tops etc.
  part-of-speech ;v-verb, n-nount, a-adj, r-adverb; s-adjective satellite
  list-of-pointers
  list-of-lemmas
  (lemma-id -1)
  gloss)

(defun wn-synset-def->string (synset)
  (let ((str-list '())
	(gloss (wn-synset-gloss synset)))
    (dolist (example (wn-gloss-list-of-examples gloss))
	    (push (int->string 0) str-list)
	    (push example str-list))
    (dolist (def (wn-gloss-list-of-defs gloss))
	    (push (int->string 1) str-list)
	    (push def str-list))
    (string-list->string str-list)))

(defun space-p (c) (eq c #\Space))
(defun copyright-line-p (s)
    (and (> (length s) 2) (space-p (elt s 0)) (space-p (elt s 1))))

(defstruct simple-lemma
  lemma
  def
  pos
  (id 0))

(defun simple-lemma->string (lemma)
  (string-list->string (reverse (simple-lemma-def lemma))))

(defstruct simple-datasource
  file-name
  (lemmas-hash (make-hash-table :size 50000 :test 'equal))
  (lemmas '())
  (sorted-lemmas '())
  (has-pos-info-p nil))
  
(defstruct wn-datasource
  dir
  (lemmas-hash (make-hash-table :size 200000 :test 'equal))
  (lemmas '())
  (demo-p '())
  (fast-p '())
  (accept-synset-hook '())
  (nouns-p 't) (verbs-p 't) (adj-p 't) (adv-p 't)
  (with-examples-p 't)
  (sorted-lemmas '())
  (max-synsets-for-word 0)
  (synset-id -1)
  (synsets '())
  (sorted-synsets '()))

(defun make-gloss (gloss)
  (let* ((strings (split-string gloss ";"))
	 (list-of-defs '())
	 (list-of-examples '()))
    (dolist (el strings)
      (cond ((= 0 (length (string-trim " " el))) nil)
	    ((string= "\"" el) nil) ; TODO: hack around ;"; thing (e.g. in "trash these old chairs" in data.verb)
	    ((string= (subseq el 0 2) " \"") (push (string-trim '(#\space #\") el) list-of-examples))
	    (t (push (string-trim " " el) list-of-defs))))
;;       (if (string= (subseq el 0 2) " \"")
;; 	  (push (string-trim '(#\space #\") el) list-of-examples)
;; 	  (push (string-trim '(#\space) el) list-of-defs)))
    (make-wn-gloss :list-of-defs list-of-defs
		   :list-of-examples list-of-examples)))

(defun wn-data-line->wn-synset (line)
  (let* ((two-strings (split-string line "|"))
	 (all (split-string (car two-strings) " "))
	 (lex-filenum (dec-string->integer (nth 1 all)))
	 (synset-type (nth 2 all))
	 (words-count (hex-string->integer (nth 3 all)))
	 (lemmas-list '())
;	 (pointer-cnt-pos (+ 3 (* 2 words-count)))
;	 (pointers-cnt (nth pointer-cnt-pos all))
	 (list-of-pointers '()))
    (dotimes (n words-count)
	 (push (list
;;		(substitute #\space #\_ (string-downcase (nth (+ 4 (* 2 n)) all)))
		(substitute #\space #\_ (nth (+ 4 (* 2 n)) all))
		(hex-string->integer (nth (+ 5 (* 2 n)) all)))
	       lemmas-list))
;    (delete-duplicates lemmas-list :test #'equal) ; this is necessary for pairs like Qiana/qiana
    (make-wn-synset :lex-filenum lex-filenum
		    :part-of-speech synset-type
		    :list-of-pointers list-of-pointers
		    :list-of-lemmas (reverse lemmas-list)
		    :gloss (make-gloss (nth 1 two-strings)))))

(defmethod wn-add-lemma ((data-source wn-datasource) lemma synset)
  (let* ((hash (slot-value data-source 'lemmas-hash))
	 (wn-lemma (gethash lemma hash)))
    (if (not wn-lemma)
	(setq wn-lemma
	      (setf (gethash lemma hash)
		    (make-wn-lemma :lemma lemma :list-of-synsets (list synset))))
      (if (not (find-if #'(lambda (s1) (= (wn-synset-id synset) (wn-synset-id s1)))
			(wn-lemma-list-of-synsets wn-lemma)))
		(push synset (wn-lemma-list-of-synsets wn-lemma))
	(format t "~&@dup found: ~A ~%" lemma)))
    (setf (slot-value data-source 'max-synsets-for-word)
	  (max (length (wn-lemma-list-of-synsets wn-lemma))
	       (slot-value data-source 'max-synsets-for-word)))))

(defmethod wn-accept-all ((synset wn-synset))
  t)

(defun rejected-count (word-hash lemma-list)
  (let ((count 0))
    (dolist (el lemma-list)
	    (if (gethash (car el) word-hash)
		(incf count)))
    count))

;; accept if all synset words are on the list (most restrictive)
(defun make-wn-accept-hook-mini (word-hash)
  (lambda (synset)
    (if (= (rejected-count word-hash (wn-synset-list-of-lemmas synset))
	   (length (wn-synset-list-of-lemmas synset)))
	't
      nil)))

(defvar *hd-called* 0)
(defvar *hd-accepted* 0)

(defun make-wn-accept-hook-demo (freq)
  (let ((counter 0))
    (lambda (synset)
      (declare (ignore synset))
      (incf *hd-called*)
      (setq counter (+ 1 counter))
      (if (= counter freq)
	  (progn
	    (setq counter 0)
	    (incf *hd-accepted*)
	    't)
	nil))))

;; accept if more than half of synset words is on the list
(defun make-wn-accept-hook-small (word-hash)
  (lambda (synset)
    (let ((rejected-count (rejected-count word-hash (wn-synset-list-of-lemmas synset))))
      (if (> rejected-count (- (length (wn-synset-list-of-lemmas synset)) rejected-count))
	  't
	nil))))
  
;; accept if at least one word is on the list
(defun make-wn-accept-hook-medium (word-hash)
  (lambda (synset)
    (if (> (rejected-count word-hash (wn-synset-list-of-lemmas synset)) 0)
	't
      nil)))

;; reverse of medium: accept those that do not belong to medium
(defun make-wn-accept-hook-advanced (word-hash)
  (lambda (synset)
    (if (> (rejected-count word-hash (wn-synset-list-of-lemmas synset)) 0)
	nil
      't)))

(defmethod wn-synset-allowed-p ((data-source wn-datasource) synset)
  (let ((accept-hook (wn-datasource-accept-synset-hook data-source)))
    (if (not (null accept-hook))
	(funcall accept-hook synset)
      't)))

(defmethod wn-add-synset ((data-source wn-datasource) synset)
  ;; filter out examples if we don't want them, this could be put
  ;; in many other places
  (if (not (wn-datasource-with-examples-p data-source))
      (setf (wn-gloss-list-of-examples (wn-synset-gloss synset)) '()))
  (if (wn-synset-allowed-p data-source synset)
      (progn
	(setf (wn-synset-id synset) (incf (slot-value data-source 'synset-id)))
	(push synset (slot-value data-source 'synsets))
	(dolist (lemma-list (wn-synset-list-of-lemmas synset))
	  (wn-add-lemma data-source (car lemma-list) synset)))))

(defmethod wn-line-process ((data-source wn-datasource) line)
    (if (not (copyright-line-p line))
      (wn-add-synset data-source (wn-data-line->wn-synset line))))

(defmethod read-wordnet-file ((data-source wn-datasource) file-type)
  (let ((full-file-name (concatenate 'string
				     (slot-value data-source 'dir)
				     "data."
				     file-type)))
    (with-open-file
     (in-stream full-file-name :direction :input)
     (do ((l (read-line in-stream) (read-line in-stream nil 'eof)))
	 ((eq l 'eof) (format t "~&Finished reading file ~A~%" full-file-name))
       (wn-line-process data-source l)))))

(defconstant SS_NONE 0)
(defconstant SS_HAS_LEMMA 1)
(defconstant SS_READING_DEF 2)

(defun state-name (state)
  (cond
   ((= state SS_NONE) "SS_NONE")
   ((= state SS_HAS_LEMMA) "SS_HAS_LEMMA")
   ((= state SS_READING_DEF) "SS_READING_DEF")
   (t (error "invalid state ~D in state-name" state))))

(defun is-pos-p (s)
  (member s '("n." "adj." "v.t." "v.i." "adv." "pp.")
	  :test #'(lambda (s1 s2) (equal (string-downcase s1) (string-downcase s2)))))

(defmethod add-simple-lemma ((data-source simple-datasource) lemma pos def)
  (let ((new-lemma
	  (make-simple-lemma :lemma lemma :pos pos :def def)))
    (if (or (null def) (equal lemma ""))
	(error "bad lemma: ~A pos: ~A " lemma pos))
    (push new-lemma (simple-datasource-lemmas data-source))
    (setf (gethash lemma (simple-datasource-lemmas-hash data-source)) new-lemma)))

(defmethod read-simple-file ((data-source simple-datasource))
  (let ((state SS_NONE)
	(def '())
	(has-pos-info-p nil)
	(lemma "")
	(pos "")
	(prev-line "")
	(prev-state SS_NONE)
	(pos-state-known-p nil))
    (with-open-file
     (in-stream (simple-datasource-file-name data-source) :direction :input)
     (do ((l (read-line in-stream) (read-line in-stream nil 'eof))
	  (line-no 1 (+ line-no 1)))
	 ((eq l 'eof) (format t "~&Finished reading file ~A~%"
			      (simple-datasource-file-name data-source)))
       ;;(format t "~&line: ~A~%" l)
       (if (is-empty-line-p l)
	   (progn
	     ;; empty line after lemma means empty definition => error
	     (if (= state SS_HAS_LEMMA)
		 (error "~%unexpected state: ~A~%previous   state: ~A~%previous line: \"~A\"~%current  line: \"~A\"~%line number: ~D"
			(state-name state) (state-name prev-state) prev-line l line-no))
	     ;; SS_NONE after SS_NONE == consequtive empty lines => just ignore
	     (if (= state SS_NONE)
		 (if (not (or (= prev-state SS_READING_DEF) (= prev-state SS_NONE)))
		     (error "~%unexpected state: ~A~%previous   state: ~A~%previous line: \"~A\"~%current  line: \"~A\"~%line number: ~D"
			    (state-name state) (state-name prev-state) prev-line l line-no)))
	     (if (= state SS_READING_DEF)
		 (progn
		   (add-simple-lemma data-source lemma pos def)
		   (setq prev-state state)
		   ;;(format t "state changed SS_NONE")
		   (setq state SS_NONE))))
	 (progn
	   ;;(format t "~&line: ~A is not empty-line~%" l)
	   ;;(format t "~&state: ~A~%" (state-name state))
	   (cond ((= state SS_NONE)
		  ;;(format t "~&line: ~A is lemma~%" l)
		  (setq lemma (string-downcase l))
		  (setq pos "")
		  (setq def "")
		  (setq prev-state state)
		  ;;(format t "state changed SS_HAS_LEMMAS")
		  (setq state SS_HAS_LEMMA))
		 ((= state SS_HAS_LEMMA)
		  ;;(format t "~&line: ~A is in has-lemma~%" l)
		  (if (not pos-state-known-p)
		      (progn
			(setq pos-state-known-p 't)
			(if (is-pos-p l)
			    (setq has-pos-info-p 't)
			  (setq has-pos-info-p nil))))
		  (if has-pos-info-p
		      (progn
			(if (not (is-pos-p l))
			    (error "~%unknown pos: ~A~%curr-state: ~A~%prev-state: ~A~%"
				   l (state-name state) (state-name prev-state)))
			(setq pos l)
			(setq def '()))
		    (setq def (list l)))
		  (setq prev-state state)
		  ;;(format t "state changed SS_READING_DEF")
		  (setq state SS_READING_DEF))
		 ((= state SS_READING_DEF)
		  ;;(format t "~&line: ~A is in reading-def~%" l)
		  (if (is-pos-p l)
		      (error "suspicious: pos (~A) inside def of: ~A, prev line:~A" l lemma prev-line))
		  (if (not (null def))
		      ;;this changes space vs. new line separating new lines
		      ;;(push " " def))
		      ;;(push "\n" def))
		      (push new-line-s def))
		  (push l def))
		 (t (error "unknown state: ~D" state)))
	   (setq prev-line l))))
    (setf (simple-datasource-has-pos-info-p data-source) has-pos-info-p))))
    ;; (format t "Finshed reading file ~A" (simple-datasource-file-name data-source)))))
  

(defmethod datasource-make-sorted-lemmas ((data-source wn-datasource))
  (let ((all-lemmas '())
	(lemma-id -1))
    (if (slot-value data-source 'nouns-p)
	(read-wordnet-file data-source "noun"))
    (if (slot-value data-source 'verbs-p)
	(read-wordnet-file data-source "verb"))
    (if (slot-value data-source 'adj-p)
	(read-wordnet-file data-source "adj"))
    (if (slot-value data-source 'adv-p)
	(read-wordnet-file data-source "adv"))
    (maphash #'(lambda (key val) (declare (ignore key)) (push val all-lemmas))
	     (wn-datasource-lemmas-hash data-source))
    (setf (wn-datasource-sorted-lemmas data-source)
	  (sort all-lemmas
		#'(lambda (x y) (string-lessp (wn-lemma-lemma x)
					      (wn-lemma-lemma y)))))
    (dolist (lemma (wn-datasource-sorted-lemmas data-source))
      (setf (wn-lemma-id lemma) (incf lemma-id)))
    (wn-datasource-sorted-lemmas data-source)))

(defmethod datasource-make-sorted-lemmas ((data-source simple-datasource))
  (let ((all-lemmas '())
	(lemma-id -1))
    (read-simple-file data-source)
    (maphash #'(lambda (key val) (declare (ignore key)) (push val all-lemmas))
	     (simple-datasource-lemmas-hash data-source))
    (setf (simple-datasource-sorted-lemmas data-source)
	  (sort all-lemmas
		#'(lambda (x y) (string-lessp (simple-lemma-lemma x)
					      (simple-lemma-lemma y)))))
    (dolist (lemma (simple-datasource-sorted-lemmas data-source))
      (setf (simple-lemma-id lemma) (incf lemma-id)))
    (simple-datasource-sorted-lemmas data-source)))
      
(defmethod lowest-word-num ((data-source wn-datasource) synset)
  (let* ((hash (wn-datasource-lemmas-hash data-source))
	 (words (synset-words synset))
	 (lowest-num (wn-lemma-id (gethash (car words) hash))))
    (dolist (lex (cdr words))
	    (setq lowest-num (min lowest-num (wn-lemma-id (gethash lex hash)))))
    lowest-num))

(defmethod wn-datasource-make-sorted-synsets ((data-source wn-datasource))
  (if (null (wn-datasource-sorted-lemmas data-source))
      (datasource-make-sorted-lemmas data-source))
  (setf (wn-datasource-sorted-synsets data-source)
	(sort (wn-datasource-synsets data-source)
	      #'(lambda (s1 s2)
		  (< (lowest-word-num data-source s1)
		     (lowest-word-num data-source s2))))))
  
(defmethod datasource-get-sorted-lemmas ((data-source wn-datasource))
  (if (not (null (wn-datasource-sorted-lemmas data-source)))
      (wn-datasource-sorted-lemmas data-source)
    (datasource-make-sorted-lemmas data-source)))

(defmethod datasource-get-sorted-lemmas ((data-source simple-datasource))
  (if (null (simple-datasource-sorted-lemmas data-source))
      (datasource-make-sorted-lemmas data-source))
  (simple-datasource-sorted-lemmas data-source))

(defmethod datasource-get-sorted-synsets ((data-source wn-datasource))
  (if (not (null (wn-datasource-sorted-synsets data-source)))
      (wn-datasource-sorted-synsets data-source)
    (wn-datasource-make-sorted-synsets data-source)))

(defmethod make-lemma-iterator ((data-source wn-datasource))
  (let ((l (datasource-get-sorted-lemmas data-source)))
    (if (null l)
	(error "empty list in make-lemma-iterator"))
    (make-list-iterator-with-function l wn-lemma-lemma)))
;;    (make-list-iterator-with-function l #'(lambda (lemma) (wn-lemma-lemma lemma)))))

(defmethod make-lemma-iterator ((data-source simple-datasource))
  (let ((l (datasource-get-sorted-lemmas data-source)))
    (if (null l)
	(error "empty list in make-lemma-iterator"))
    (make-list-iterator-with-function l simple-lemma-lemma)))
;;    (make-list-iterator-with-function l simple-lemma-lemma)))

(defun test-wordnet ()
  (let* ((wn-source (make-wn-datasource :dir *wordnet-dir*))
	 (li (make-lemma-iterator wn-source)))
    (dotimes (n 10)
      (funcall li 'get-next))))

(defstruct wn-records
  (defs-records '())
  (defs-lens-records '())
  (max-def-len 0)
  (max-compressed-def-len 0)
  (max-words-per-synset 0)
  (bytes-per-word 2)
  (synset-info-records '())
  (defs-pack-info-rec (make-instance 'pdb-record))
  (words-nums-records '()))

;; given list of sorted wn-synsets produce the following records:
;; - for each synset size of the compressed definition (1 byte, if bigger than 254
;;   then 3 bytes: 255+longeur
;; - for each synset compressed definition
;; and following info:
;; - size of the longest uncompressed definition so that I can allocate
;;   a buffer big enough in the program (TODO: use extensible buffer?)
(defmethod make-wn-synsets-data-records (sorted-synsets (wn-recs wn-records))
  (let* ((defs-pack-info (make-pack-info))
	 (defs-records (make-instance 'pdb-record-list))
	 (max-def-len 0)
	 (max-compressed-def-len 0)
	 (uncompressed-len 0)
	 (compressed-len 0)
	 (defs-lens-records (make-instance 'pdb-record-list)))
    (dolist (synset sorted-synsets)
      (pack-info-add-string defs-pack-info (wn-synset-def->string synset)))
    (format t "~& building pack data for ~D synset defs ~%" (length sorted-synsets))
    (pack-info-build-pack-data defs-pack-info)
    (dolist (synset sorted-synsets)
      (let* ((def (wn-synset-def->string synset))
	     (compr-def (pack-string defs-pack-info def)))
	(setq uncompressed-len (+ uncompressed-len (length def)))
	(setq compressed-len (+ compressed-len (length compr-def)))
	(setq max-def-len (max max-def-len (length def)))
	(setq max-compressed-def-len (max max-compressed-def-len (length compr-def)))
	(pdb-record-add-string defs-records compr-def)
	(if (< (length compr-def) 255)
	    (pdb-record-add-int8 defs-lens-records (length compr-def))
	  (progn
	    (pdb-record-add-int8 defs-lens-records 255)
	    (pdb-record-add-int16 defs-lens-records (length compr-def))))))
    (setf (wn-records-defs-lens-records wn-recs)
	  (pdb-record-list-get-records defs-lens-records))
    (setf (wn-records-defs-records wn-recs)
	  (pdb-record-list-get-records defs-records))
    (setf (wn-records-max-def-len wn-recs) max-def-len)
    (setf (wn-records-defs-pack-info-rec wn-recs)
	  (make-compression-data-record defs-pack-info))
    (format t "~& Number of synsets: ~D ~%" (length sorted-synsets))
    (format t "~& Total size of synset def data: ~D ~%" (pdb-record-size defs-records))
    (format t "~& Total size of uncompressed def: ~D ~%" uncompressed-len)
    (format t "~& Total size of compressed def: ~D ~%" compressed-len)
    (format t "~& Size of defs compression data record: ~D ~%"
	    (pdb-record-size (wn-records-defs-pack-info-rec wn-recs)))
    (format t "~& Total size of synset defs len data: ~D ~%"
	    (pdb-record-size defs-lens-records))))

(defmethod synset-words ((synset wn-synset))
  (let ((words '()))
    (dolist (wn-lemma (wn-synset-list-of-lemmas synset))
      (push (car wn-lemma) words))
    (reverse words)))

(defmethod synset-pos ((syn wn-synset))
  (let ((pos (wn-synset-part-of-speech syn)))
    (cond ((equal pos "r") "adv")
	  ((equal pos "a") "adj")
	  ((equal pos "s") "adj")
	  (t pos))))

(defmethod lemma-pos ((lemma simple-lemma))
  (let ((pos (simple-lemma-pos lemma)))
    (cond ((equal pos "n.") "n")
	  ((equal pos "adj.") "adj")
	  ((equal pos "adv.") "adv")
	  ((equal pos "v.t.") "v")
	  ((equal pos "v.i.") "v")
	  ((equal pos "pp.") "v")
	  (t (error "unknown pos: ~A" pos)))))

(defun set-pos (pos pos-accumulated pos-idx)
  (cond ((equal pos "n") pos-accumulated)
	((equal pos "v") (set-bit-on-integer pos-accumulated (* 2 pos-idx)))
	((equal pos "adj") (set-bit-on-integer pos-accumulated (+ 1 (* 2 pos-idx))))
	((equal pos "adv")
	 (set-bit-on-integer (set-bit-on-integer pos-accumulated (+ 1 (* 2 pos-idx)))
			     (* 2 pos-idx)))
	(t (error "unknow pos ~S in set-pos" pos))))

;; given list of sorted wn-synsets and hash with all words produce
;; the following records:
;; - for each synset words that belong to this synset
;; - for each synset a byte that encodes:
;;   - number of words in this synset in 6 lower bits (means 63 is max)
;;   - part of speech in upper 2 bits
(defmethod make-wn-synsets-records (hash sorted-synsets (wn-recs wn-records))
  (let* ((words-nums-recs (make-instance 'pdb-record-list))
	 (bytes-per-word 2)
	 (total-words-count 0)
	 (max-words-per-synset 0)
	 (words-count-and-pos 0)
	 (synset-info-recs (make-instance 'pdb-record-list)))
    (if (> (hash-table-count hash) 65535)
	(setq bytes-per-word 3))
    (dolist (synset sorted-synsets)
      (let* ((words (wn-synset-list-of-lemmas synset))
	     (words-count (length words)))
	(if (= 0 words-count)
	    (error "words-count is 0"))
	(setq max-words-per-synset (max max-words-per-synset words-count))
	(setq total-words-count (+ words-count total-words-count))
	(if (> (+ (current-record-size words-nums-recs)
		  (* bytes-per-word words-count)) +max-record-len+)
	    (pdb-record-list-new-record words-nums-recs))
	(setq words-count-and-pos (set-pos (synset-pos synset) words-count 3))
	(pdb-record-add-int8 synset-info-recs words-count-and-pos)
	(dolist (lex (synset-words synset))
	  (if (= 2 bytes-per-word)
	      (pdb-record-add-int16 words-nums-recs (wn-lemma-id (gethash lex hash))))
	  (if (= 3 bytes-per-word)
	      (pdb-record-add-int24 words-nums-recs (wn-lemma-id (gethash lex hash)))))))
    (setf (wn-records-words-nums-records wn-recs)
	  (pdb-record-list-get-records words-nums-recs))
    (setf (wn-records-synset-info-records wn-recs)
	  (pdb-record-list-get-records synset-info-recs))
    (setf (wn-records-bytes-per-word wn-recs) bytes-per-word)
    (setf (wn-records-max-words-per-synset wn-recs) max-words-per-synset)
    (format t "~& Number of synsets: ~D ~%" (length sorted-synsets))
    (format t "~& Number of unique words: ~D ~%" (hash-table-count hash))
    (format t "~& Total number of words: ~D ~%" total-words-count)
    (format t "~& Bytes per word: ~D ~%" bytes-per-word)
    (format t "~& Synsets info recs size: ~D ~%" (* bytes-per-word total-words-count))
    (format t "~& max-words-per-synset: ~D ~%" max-words-per-synset)))

(defstruct simple-def-records
  (defs-lens-records '())
  (defs-records '())
  (max-def-len 0)
  (pos-recs '())
  (defs-pack-info-rec '()))

;; given list of sorted simple-defs produce the following records:
;; - for each def size of the compressed definition (1 byte, if bigger than 254
;;   then 3 bytes: 255+longeur
;; - for each def compressed definition
;; and following info:
;; - size of the longest uncompressed definition so that I can allocate
;;   a buffer big enough in the program
(defmethod make-simple-def-data-records ((data-source simple-datasource) def-recs)
  (let* ((defs-pack-info (make-pack-info))
	 (defs-records (make-instance 'pdb-record-list))
	 (max-def-len 0)
	 (sorted-lemmas (datasource-get-sorted-lemmas data-source))
	 (pos-recs (make-instance 'pdb-record-list))
	 (max-compressed-def-len 0)
	 (uncompressed-len 0)
	 (pos-byte 0)
	 (pos-idx 0)
	 (compressed-len 0)
	 (defs-lens-records (make-instance 'pdb-record-list)))
    (dolist (lemma sorted-lemmas)
      (pack-info-add-string defs-pack-info (simple-lemma->string lemma)))
    (format t "~& building pack data for ~D lemmas defs ~%" (length sorted-lemmas))
    (pack-info-build-pack-data defs-pack-info)
    (format t "~& compressin lemmas defs ~%")
    (dolist (lemma sorted-lemmas)
      (let* ((def (simple-lemma->string lemma))
	     (compr-def (pack-string defs-pack-info def)))
	(setq uncompressed-len (+ uncompressed-len (length def)))
	(setq compressed-len (+ compressed-len (length compr-def)))
	(setq max-def-len (max max-def-len (length def)))
	(setq max-compressed-def-len (max max-compressed-def-len (length compr-def)))
	(pdb-record-add-string defs-records compr-def)
	(if (simple-datasource-has-pos-info-p data-source)
	    (setq pos-byte (set-pos (lemma-pos lemma) pos-byte pos-idx)))
	(if (= 3 pos-idx)
	    (progn
	      (pdb-record-add-int8 pos-recs pos-byte)
	      (setq pos-byte 0)
	      (setq pos-idx 0))
	  (if (simple-datasource-has-pos-info-p data-source)
	      (setq pos-idx (+ 1 pos-idx))))
	(if (< (length compr-def) 255)
	    (pdb-record-add-int8 defs-lens-records (length compr-def))
	  (progn
	    (pdb-record-add-int8 defs-lens-records 255)
	    (pdb-record-add-int16 defs-lens-records (length compr-def))))))
    (if (not (= 0 pos-idx))
	(pdb-record-add-int8 pos-recs pos-byte))
    (setf (simple-def-records-defs-lens-records def-recs)
	  (pdb-record-list-get-records defs-lens-records))
    (setf (simple-def-records-defs-records def-recs)
	  (pdb-record-list-get-records defs-records))
    (setf (simple-def-records-max-def-len def-recs) max-def-len)
    (setf (simple-def-records-pos-recs def-recs) (pdb-record-list-get-records pos-recs))
    (setf (simple-def-records-defs-pack-info-rec def-recs)
	  (make-compression-data-record defs-pack-info))
    (format t "~& Number of words: ~D ~%" (length sorted-lemmas))
    (format t "~& Total size of synset def data: ~D ~%" (pdb-record-size defs-records))
    (format t "~& Total size of uncompressed def: ~D ~%" uncompressed-len)
    (format t "~& Total size of compressed def: ~D ~%" compressed-len)
    (format t "~& Size of defs compression data record: ~D ~%"
	    (pdb-record-size (simple-def-records-defs-pack-info-rec def-recs)))
    (format t "~& Total size of synset defs len data: ~D ~%"
	    (pdb-record-size defs-lens-records))))

(defun do-simple (simple-source pdb-name out-filename db-type)
  (let* ((pdb (make-pdb))
	 (iterator (make-lemma-iterator simple-source))
	 (simple-recs (make-simple-def-records))
	 (first-record (make-instance 'pdb-record))
	 ;;(cur-record 0)
	 (word-packer (make-word-packer)))
    ;;(setf (simple-datasource-file-name simple-source) "d.txt")
    (setf (pdb-name pdb) pdb-name)
    (setf (pdb-creator-id pdb) "NoAH")
    (setf (pdb-db-type pdb) db-type)
    (pdb-set-dates pdb)
    (word-packer-build-word-pdb-data word-packer iterator)
    (word-packer-dump-info word-packer)
    (make-simple-def-data-records simple-source simple-recs)
    ;; number of words
    (pdb-record-add-int32 first-record
			  (hash-table-count
			   (simple-datasource-lemmas-hash simple-source)))
    ;; number of words records
    (pdb-record-add-int16 first-record
			  (length (word-packer-words-records word-packer)))
    ;; number of def lens records
    (pdb-record-add-int16 first-record
			  (length (simple-def-records-defs-lens-records simple-recs)))
    ;; number of def records
    (pdb-record-add-int16 first-record
			  (length (simple-def-records-defs-records simple-recs)))
    ;; max_word_len
    (pdb-record-add-int16 first-record (word-packer-max-word-len word-packer))
    ;; max-def-len
    (pdb-record-add-int16 first-record (simple-def-records-max-def-len simple-recs))
    ;; has pos info
    (if (simple-datasource-has-pos-info-p simple-source)
	(pdb-record-add-int16 first-record 1)
      (pdb-record-add-int16 first-record 0))
    ;; number of pos records
    (if (simple-datasource-has-pos-info-p simple-source)
	(pdb-record-add-int16 first-record
			      (length (simple-def-records-pos-recs simple-recs)))
      (pdb-record-add-int16 first-record 0))
    ;; first record
    (pdb-add-record pdb first-record)
    ;; word cache
    (pdb-add-record pdb (get-word-cache-info-pdb-record word-packer))
    ;; words pack info record
    (pdb-add-record pdb
		    (make-compression-data-record (word-packer-pack-info word-packer)))
    ;; def pack info record
    (pdb-add-record pdb (simple-def-records-defs-pack-info-rec simple-recs))
    ;; words
    (dolist (rec (reverse (word-packer-words-records word-packer)))
      (pdb-add-record pdb rec))
    ;; def lens
    (dolist (rec (simple-def-records-defs-lens-records simple-recs))
      (pdb-add-record pdb rec))
    ;; defs
    (format t "~& Number of def records: ~D ~%"
	    (length (simple-def-records-defs-records simple-recs)))
    (dolist (rec (simple-def-records-defs-records simple-recs))
      (pdb-add-record pdb rec))
    ;; pos recs
    (if (simple-datasource-has-pos-info-p simple-source)
	(progn
	  (format t "~& Number of def records: ~D ~%"
		  (length (simple-def-records-pos-recs simple-recs)))
	  (dolist (rec (simple-def-records-pos-recs simple-recs))
	    (pdb-add-record pdb rec))))
    (write-pdb out-filename pdb)))

(defmethod highest-word-num ((data-source wn-datasource) synset)
  (let* ((hash (wn-datasource-lemmas-hash data-source))
	 (words (synset-words synset))
	 (num (wn-lemma-id (gethash (car words) hash))))
    (dolist (lex (cdr words))
	    (setq num (max num (wn-lemma-id (gethash lex hash)))))
    num))

(defun wn-check ()
  (let* ((sorted-synsets '())
	 (count 0)
	 (cur-len 0)
	 (wn-sorted '())
	 (wn-source (make-wn-datasource :dir *wordnet-dir*)))
    (format t "~& sorting phase 1~%")
    (setq sorted-synsets (datasource-get-sorted-synsets wn-source))
    (format t "~& sorting phase 2~%")
    (setq wn-sorted (sort sorted-synsets
			  #'(lambda (s1 s2)
			      (< (length (synset-words s1))
				 (length (synset-words s2))))))
    (setq cur-len (length (synset-words (car wn-sorted))))
    (dolist (syn wn-sorted)
	    (if (= cur-len (length (synset-words syn)))
		(setq count (+ 1 count))
	      (progn
		(format t "~& len: ~D count: ~D" cur-len count)
		(setq count 1)
		(setq cur-len (length (synset-words syn))))))))

(defun wordnet-test-max ()
  (let* (;;(hash (make-engpol-words-hash))
	 ;;(accept-mini (make-wn-accept-hook-mini hash))
	 (sorted-synsets '())
	 (max -1)
	 (current 0)
	 (entries 0)
	 (wn-source (make-wn-datasource :dir *wordnet-dir*)))
    ;;(setf (wn-datasource-accept-synset-hook wn-source) accept-mini)
    (setq  sorted-synsets (datasource-get-sorted-synsets wn-source))
    (dolist (syn sorted-synsets)
      (if (> (highest-word-num wn-source syn) max)
	  (progn
	    (setq max (highest-word-num wn-source syn))
	    (setq entries (+ 1 entries))
	    (format t "~& syn: ~D word: ~D ~%" current max)))
      (setq current (+ current 1)))
    (format t "~& total words  : ~D ~%"
	    (hash-table-count (wn-datasource-lemmas-hash wn-source)))
    (format t "~& total synsets: ~D ~%" (length sorted-synsets))
    (format t "~& entries: ~D ~%" entries)))

(defun set-2-bit-num-on-integer (int-in num idx)
  (cond ((= num 0) int-in)
	((= num 1) (set-bit-on-integer int-in (* 2 idx)))
	((= num 2) (set-bit-on-integer int-in (+ 1 (* 2 idx))))
	((= num 3) (set-bit-on-integer
		    (set-bit-on-integer int-in (+ 1 (* 2 idx)))
		    (* 2 idx)))
	(t (error "invalid num: ~D should be 0..3" num))))

(defun make-fast-record (sorted-lemmas)
  (let* ((rec (make-instance 'pdb-record))
	 (num 0)
	 (idx 0))
    (format t "~& number of lemmas: ~D ~%" (length sorted-lemmas))
    (dolist (lemma sorted-lemmas)
	    (let ((syn-count (length (wn-lemma-list-of-synsets lemma))))
	      (if (< syn-count 4)
		  (setq num (set-2-bit-num-on-integer num syn-count idx)))
	      (setq idx (+ 1 idx))
	      (if (= idx 4)
		  (progn
		    (pdb-record-add-int8 rec num)
		    (setq idx 0)
		    (setq num 0)))))
    (if (not (= 0 idx))
	(pdb-record-add-int8 rec num))
    (format t "~& size of fast-record: ~D ~%" (pdb-record-size rec))
    (format t "~& size that should be: ~D ~%" (/ (length sorted-lemmas) 4))
    rec))


(defun make-word-count-cache-info (rec wn-source cache-span)
  (let* ((sorted-synsets (datasource-get-sorted-synsets wn-source))
	 (total-words-count 0)
	 (cache-entries 0)
	 (cur-synset 1)
	 (cur-idx cache-span))
    (dolist (syn sorted-synsets)
      (setq total-words-count (+ total-words-count (length (wn-synset-list-of-lemmas syn))))
      (setq cur-idx (- cur-idx 1))
      (if (= 0 cur-idx)
	  (progn
	    (setq cur-idx cache-span)
	    (incf cache-entries)
	    (pdb-record-add-int32 rec cur-synset)
	    (pdb-record-add-int32 rec total-words-count)))
      (setq cur-synset (+ 1 cur-synset)))
    (pdb-record-add-int32 rec cur-synset)
    (pdb-record-add-int32 rec total-words-count)
    (format t "~& fast cache entries: ~D ~%" cache-entries)))

(defun do-wordnet (wn-source pdb-name out-file-name db-type)
  (let* ((pdb (make-pdb))
	 (iterator (make-lemma-iterator wn-source))
	 (wn-recs (make-wn-records))
	 (first-record (make-instance 'pdb-record))
	 (cur-record 0)
	 (word-packer (make-word-packer)))
    (setf (pdb-name pdb) pdb-name)
    (setf (pdb-creator-id pdb) "NoAH")
    (setf (pdb-db-type pdb) db-type)
    (pdb-set-dates pdb)
    (word-packer-build-word-pdb-data word-packer iterator)
    (word-packer-dump-info word-packer)
    (make-wn-synsets-records (wn-datasource-lemmas-hash wn-source)
			     (datasource-get-sorted-synsets wn-source) wn-recs)
    (make-wn-synsets-data-records (datasource-get-sorted-synsets wn-source) wn-recs)
    ;; words_count
    (pdb-record-add-int32 first-record
			  (hash-table-count (wn-datasource-lemmas-hash wn-source)))
    ;; synsets_count
    (pdb-record-add-int32 first-record
			  (length (wn-datasource-sorted-synsets wn-source)))
    ;; words_recs_count
    (pdb-record-add-int16 first-record
			  (length (word-packer-words-records word-packer)))
    ;; synsets_info_recs_count
    (pdb-record-add-int16 first-record
			  (length (wn-records-synset-info-records wn-recs)))
    ;; words_num_recs_count
    (pdb-record-add-int16 first-record
			  (length (wn-records-words-nums-records wn-recs)))
    ;; defs_len_recs_count
    (pdb-record-add-int16 first-record
			  (length (wn-records-defs-lens-records wn-recs)))
    ;; defs_recs_count
    (pdb-record-add-int16 first-record
			  (length (wn-records-defs-records wn-recs)))
    ;; max_word_len
    (pdb-record-add-int16 first-record
			  (word-packer-max-word-len word-packer))
    ;; max_def_len
    (pdb-record-add-int16 first-record
			  (wn-records-max-def-len wn-recs))
    ;; max_compr_def_len
    (pdb-record-add-int16 first-record
			  (wn-records-max-def-len wn-recs))
    ;; bytes_per_word_num
    (pdb-record-add-int16 first-record
			  (wn-records-bytes-per-word wn-recs))
    ;; max_words_per_synset
    (pdb-record-add-int16 first-record
			  (wn-records-max-words-per-synset wn-recs))
    (if (wn-datasource-fast-p wn-source)
	(make-word-count-cache-info first-record wn-source 1000))
    ;; first record
    (setq cur-record 0)
    (format t "~& rec ~D first record ~%" cur-record)
    (pdb-add-record pdb first-record)
    ;; word cache
    (pdb-add-record pdb (get-word-cache-info-pdb-record word-packer))
    ;; for demo defs pack info, words pack info
    ;; for normal vice-versa
    (if (wn-datasource-demo-p wn-source)
	(progn
	  (pdb-add-record pdb (wn-records-defs-pack-info-rec wn-recs))
	  (pdb-add-record pdb (make-compression-data-record (word-packer-pack-info word-packer))))
      (progn
	  (pdb-add-record pdb (make-compression-data-record (word-packer-pack-info word-packer)))
	  (pdb-add-record pdb (wn-records-defs-pack-info-rec wn-recs))))
    (setq cur-record 3)
    ;; compressed words
    (setq cur-record (+ 1 cur-record))
    (format t "~& rec ~D first words ~%" cur-record)
    (dolist (rec (reverse (word-packer-words-records word-packer)))
      (pdb-add-record pdb rec))
    ;; synset info recs
    (setq cur-record (+ cur-record (length (word-packer-words-records word-packer))))
    (format t "~& rec ~D first synset info ~%" cur-record)
    (dolist (rec (wn-records-synset-info-records wn-recs))
      (pdb-add-record pdb rec))
    ;; words num recs
    (setq cur-record (+ cur-record (length (wn-records-synset-info-records wn-recs))))
    (format t "~& rec ~D first words nums ~%" cur-record)
    (dolist (rec (wn-records-words-nums-records wn-recs))
      (pdb-add-record pdb rec))
    ;; defs lens
    (setq cur-record (+ cur-record (length (wn-records-words-nums-records wn-recs))))
    (format t "~& rec ~D first defs lens ~%" cur-record)
    (dolist (rec (wn-records-defs-lens-records wn-recs))
      (pdb-add-record pdb rec))
    ;; defs
    (setq cur-record (+ cur-record (length (wn-records-defs-lens-records wn-recs))))
    (format t "~& rec ~D first defs ~%" cur-record)
    (dolist (rec (wn-records-defs-records wn-recs))
      (pdb-add-record pdb rec))
    (setq cur-record (+ cur-record (length (wn-records-defs-records wn-recs))))

    (if (wn-datasource-fast-p wn-source)
	(pdb-add-record pdb (make-fast-record (datasource-get-sorted-lemmas wn-source))))

    (format t "~& rec ~D last+1 ~%" cur-record)
    (write-pdb out-file-name pdb)
    (format t "~&Written all~%")))

(defun do-wordnet-full (db-name out-file-name &optional (with-examples-p 't) (fast-p nil))
  (let* ((wn-source (make-wn-datasource :dir *wordnet-dir*)))
    (setf (wn-datasource-with-examples-p wn-source) with-examples-p)
    (setf (wn-datasource-fast-p wn-source) fast-p)
    (do-wordnet wn-source db-name out-file-name "wn20")))

(defun do-wordnet-adj (db-name out-file-name &optional (with-examples-p 't))
  (let* ((wn-source (make-wn-datasource :dir *wordnet-dir*)))
    (setf (wn-datasource-with-examples-p wn-source) with-examples-p)
    (setf (wn-datasource-nouns-p wn-source) nil)
    (setf (wn-datasource-verbs-p wn-source) nil)
    (setf (wn-datasource-adv-p wn-source) nil)
    (do-wordnet wn-source db-name out-file-name "wn20")))

(defun do-wordnet-limit (accept-hook db-name out-file-name &optional (with-examples-p 't) (fast-p nil))
  (let* ((wn-source (make-wn-datasource :dir *wordnet-dir*)))
    (setf (wn-datasource-with-examples-p wn-source) with-examples-p)
    (setf (wn-datasource-accept-synset-hook wn-source) accept-hook)
    (setf (wn-datasource-fast-p wn-source) fast-p)
    (do-wordnet wn-source db-name out-file-name "wn20")))

(defun do-wordnet-demo (db-name out-file-name)
  (let* ((wn-source (make-wn-datasource :dir *wordnet-dir*))
	 (accept-demo (make-wn-accept-hook-demo 20)))
    (setf (wn-datasource-with-examples-p wn-source) 't)
    (setf (wn-datasource-demo-p wn-source) 't)
    (setf (wn-datasource-accept-synset-hook wn-source) accept-demo)
    (setf (wn-datasource-fast-p wn-source) 't)
    (do-wordnet wn-source db-name out-file-name "wnde")))

(defun do-wordnet-full-both ()
  (do-wordnet-full "WN full" "wn_full.pdb")
  (do-wordnet-full "WN full lite" "wn_full_lite.pdb" nil))

(defun do-both-adj ()
  (do-wordnet-adj "Wordnet adj" "wordnet_adj.pdb")
  (do-wordnet-adj "Wordnet adj no ex" "wordnet_adj_no_ex.pdb" nil))

(defun do-devil ()
  (let ((ds (make-simple-datasource :file-name "d.txt")))
    (do-simple ds "Devil's dictionary" "devil.pdb" "simp")))

(defun do-3gpp ()
  (let ((ds (make-simple-datasource :file-name "3GPP_Vocabulary.txt")))
    (do-simple ds "3GPP Vocabulary" "3gppvoc.pdb" "simp")))

(defun do-3gpp-abr ()
  (let ((ds (make-simple-datasource :file-name "3GPP_Abbreviations.txt")))
    (do-simple ds "3GPP Abbreviations" "3gppabbr.pdb" "simp")))

(defun do-3gpp-all ()
  (let ((ds (make-simple-datasource :file-name "3gpp_all.txt")))
    (do-simple ds "3GPP All" "3gppall.pdb" "simp")))

(defun do-bible ()
  (let ((ds (make-simple-datasource :file-name "bible_simple.txt")))
    (do-simple ds "Bible dictionary" "../bible.pdb" "simp")))

(defun do-dict ()
  (let ((ds (make-simple-datasource :file-name "dict.txt")))
    (do-simple ds "Dict" "../dict.pdb" "simp")))

(defun do-other ()
  (let ((ds (make-simple-datasource :file-name "small_dict2.txt")))
    (do-simple ds "Sample dictionary" "sample.pdb" "simp")))

(defun do-harvey ()
  (let ((ds (make-simple-datasource :file-name "arslexis_db.txt")))
    (do-simple ds "Geo" "harvey.pdb" "simp")))

(defun do-mini ()
    (let* ((hash (make-engpol-words-hash))
           (accept-mini (make-wn-accept-hook-mini hash)))
        (do-wordnet-limit accept-mini "WN mini" "wn_mini.pdb")))

(defun do-all ()
  (let* ((hash (make-engpol-words-hash))
	(accept-mini (make-wn-accept-hook-mini hash))
	(accept-medium (make-wn-accept-hook-medium hash))
	(accept-advanced (make-wn-accept-hook-advanced hash))
	(accept-small (make-wn-accept-hook-small hash)))
;;    (do-devil)
    (do-wordnet-limit accept-mini "WN mini" "wn_mini.pdb")
    (do-wordnet-limit accept-mini "WN mini lite" "wn_mini_lite.pdb" nil)
    (do-wordnet-limit accept-small "WN small" "wn_small.pdb")
    (do-wordnet-limit accept-small "WN small lite" "wn_small_lite.pdb" nil)
    (do-wordnet-limit accept-medium "WN  medium" "wn_medium.pdb")
    (do-wordnet-limit accept-medium "WN medium lite" "wn_medium_lite.pdb" nil)
    (do-wordnet-limit accept-advanced "WN advanced" "wn_advanced.pdb")
    (do-wordnet-limit accept-advanced "WN advanced lite" "wn_advanced_lite.pdb" nil)
    (do-wordnet-full "WN full" "wn_full.pdb" 't)
    (do-wordnet-full "WN full lite" "wn_full_lite.pdb" nil))
    (do-wordnet-demo "WN demo" "wn_demo.pdb"))

(defun do-wn_small ()
  (let* ((hash (make-engpol-words-hash))
	(accept-small (make-wn-accept-hook-small hash)))
    (do-wordnet-limit accept-small "WN small" "wn_small.pdb")))

;;(defun do-special ()
;;  (let* ((hash (make-special-words-hash))
;;	 (accept-special (make-wn-accept-hook-advanced hash)))
;;   (format t "~&hash-table-size: ~D~%" (hash-table-count hash))
;;    (do-wordnet-limit accept-special "WN Cathy" "wn_cathy.pdb" 't 't)))

(defun do-all-fast ()
  (let* ((hash (make-engpol-words-hash))
	(accept-mini (make-wn-accept-hook-mini hash))
	(accept-medium (make-wn-accept-hook-medium hash))
	(accept-advanced (make-wn-accept-hook-advanced hash))
	(accept-small (make-wn-accept-hook-small hash)))
;;    (do-devil)
    (do-wordnet-limit accept-mini "WN mini" "wn_mini_f.pdb" 't 't)
    (do-wordnet-limit accept-mini "WN mini lite" "wn_mini_lite_f.pdb" nil 't)
    (do-wordnet-limit accept-small "WN small" "wn_small_f.pdb" 't 't)
    (do-wordnet-limit accept-small "WN small lite" "wn_small_lite_f.pdb" nil 't)
    (do-wordnet-limit accept-medium "WN  medium" "wn_medium_f.pdb" 't 't)
    (do-wordnet-limit accept-medium "WN medium lite" "wn_medium_lite_f.pdb" nil 't)
    (do-wordnet-limit accept-advanced "WN advanced" "wn_advanced_f.pdb" 't 't)
    (do-wordnet-limit accept-advanced "WN advanced lite" "wn_advanced_lite_f.pdb" nil 't)
    (do-wordnet-full "WN full" "wn_full_f.pdb" 't 't)
    (do-wordnet-full "WN full lite" "wn_full_lite_f.pdb" nil 't))
    (do-wordnet-demo "WN demo" "wn_demo_f.pdb"))


(defvar *sds* nil)

(defun test-simple ()
  (let ((ds (make-simple-datasource :file-name "d.txt")))
    (datasource-get-sorted-lemmas ds)
    (setq *sds* ds)
    "finished"))

(defun test-simple-2 ()
  (let ((ds (make-simple-datasource :file-name "arslexis_db.txt")))
    (datasource-get-sorted-lemmas ds)
    (setq *sds* ds)
    "finished"))

(defun do-plant-orig ()
  (let ((ds (make-simple-datasource :file-name "plantDBSyn.dat")))
    (do-simple ds "Plants" "plant_orig.pdb" "simp")))

(defun do-cro-dict ()
  (let ((ds (make-simple-datasource :file-name "crodic_1.txt")))
    (do-simple ds "CroDict" "crodict.pdb" "simp")))

(defun do-plant-mine ()
  (let ((ds (make-simple-datasource :file-name "plantdb2.txt")))
    (do-simple ds "Plants" "plant_better.pdb" "simp")))

(defun do-polang-rob ()
  (let ((ds (make-simple-datasource :file-name "Pol-Eng.txt")))
    (do-simple ds "Pol-Eng" "pol-eng.pdb" "simp")))

(defun do-ddlatint ()
  (let ((ds (make-simple-datasource :file-name "ddlatin.txt")))
    (do-simple ds "latin" "lating.pdb" "simp")))

(defun do-plant-dict ()
  (let ((ds (make-simple-datasource :file-name "plant_dictionary.txt")))
    (do-simple ds "plant" "plant.pdb" "simp")))

(defun do-plant-2-dict ()
  (let ((ds (make-simple-datasource :file-name "plant.txt")))
    (do-simple ds "Plant" "plant.pdb" "simp")))


(defun do-new-plant-1 ()
  (let ((ds (make-simple-datasource :file-name "AK_PlantCodeAll.txt")))
    (do-simple ds "AK_Code" "plant-00.pdb" "simp")))

(defun do-new-plant-2 ()
  (let ((ds (make-simple-datasource :file-name "AK_PlantNameAll.txt")))
    (do-simple ds "AK_Name" "plant-01.pdb" "simp")))

(defun do-new-plant-3 ()
  (let ((ds (make-simple-datasource :file-name "PlantCodeVasc.txt")))
    (do-simple ds "CodeVasc" "plant-02.pdb" "simp")))

(defun do-new-plant-4 ()
  (let ((ds (make-simple-datasource :file-name "PlantNameVasc.txt")))
    (do-simple ds "NameVasc" "plant-03.pdb" "simp")))

(defun do-chem ()
  (let ((ds (make-simple-datasource :file-name "chemistry-2.txt")))
    (do-simple ds "chem" "chemistry.pdb" "simp")))

(defun do-new-plants ()
  (do-new-plant-1)
  (do-new-plant-2)
  (do-new-plant-3)
  (do-new-plant-4))

(defun do-newer-plant-1 ()
  (let ((ds (make-simple-datasource :file-name "AK_CodeAll_6_13_02.txt")))
    (do-simple ds "AK_Code" "plant2-00.pdb" "simp")))

(defun do-newer-plant-2 ()
  (let ((ds (make-simple-datasource :file-name "AK_NameAll_6_13_02.txt")))
    (do-simple ds "AK_Name" "plant2-01.pdb" "simp")))

(defun do-newer-plant-3 ()
  (let ((ds (make-simple-datasource :file-name "CodeVasc_6_13_02.txt")))
    (do-simple ds "CodeVasc" "plant2-02.pdb" "simp")))

(defun do-newer-plant-4 ()
  (let ((ds (make-simple-datasource :file-name "NameVasc_6_13_02.txt")))
    (do-simple ds "NameVasc" "plant2-03.pdb" "simp")))

(defun do-newer-plants ()
  (do-newer-plant-1)
  (do-newer-plant-2)
  (do-newer-plant-3)
  (do-newer-plant-4))

