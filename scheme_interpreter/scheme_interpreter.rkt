; Project 1 - Scheme interpreter for λ0
; Mihail Dunaev, 325CC
; 25 Mar 2012
;
; Assignment Instructions (RO) http://elf.cs.pub.ro/pp/teme2012/t1

; my eval has 3 types of expressions:
; tip0 - symbol, tip1 - uncalled procedure, tip2 - called procedure, else - anything else

(define (tip0? expr)
  (symbol? expr)
  )

(define (tip1? expr)
  (if (list? expr) (eq? (car expr) 'lambda0) #f)
  )

(define (tip2? expr)
  (if (list? expr) 
      (if (list? (car expr)) 
          (if (tip1? (car expr))
              #t 
              (tip2? (car expr))) 
          #f)
      #f)
  )

; eval0 routine
(define (eval0 expr) 
  (cond 
    ((tip0? expr) expr)
    ((tip1? expr) (list 'lambda0 (cadr expr) (eval0 (caddr expr))))
    ((tip2? expr) (eval0 (procedure-call expr)))
    (else (map eval0 expr))
    )
  )

; procedure-call - the most important part of the interpreter
(define (procedure-call expr) 
  (if (tip1? (car expr)) (substitute (caddar expr) (cadar expr) (cadr expr)) 
      (procedure-call (list (procedure-call (car expr)) (cadr expr))))
  )

; substitute - replaces every occurence of arg in body with val
(define (substitute body arg val) 
  (cond 
    ((tip0? body) (if (eq? body arg) val body))
    ((tip1? body) (if (eq? arg (cadr body)) body 
                      (if (myContains val (cadr body)) (substitute (alpha-conversion body) arg val) 
                          (list 'lambda0 (cadr body) (substitute (caddr body) arg val)))))
    (else (map (lambda (x) (substitute x arg val)) body))
    )
  )

; alpha-conversion - simulates a beta-reduction with a randomly generated symbol
(define (alpha-conversion body)
  (let 
      ((sym (gensym "a")))
    (list 'lambda0 sym (procedure-call (list body sym)))
      )
  )

; procedure that checks if an element is in a list or not - for alpha-conversions
(define (myContains l element)
  (if (symbol? l) (eq? l element) 
      (if (empty? l) #f (if (eq? (car l) element) #t (myContains (cdr l) element) ))
      )
  )