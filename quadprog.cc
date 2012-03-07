#include <stdlib.h>
#include <math.h>

#include <list>

#include <v8.h>
#include <node.h>

using namespace v8;
using namespace node;

extern void qpgen1(
	double *dmat, double *dvec, int *fddmat, int *n,
	double *sol, double *lagr, double *crval,
	double *amat, int *iamat, double *bvec, int *fdamat, int *q,
	int *meq, int *iact, int *nact, int *iter,
	double *work, int *ierr
);

extern void qpgen2_(
	double *dmat, double *dvec, int *fddmat, int *n,
	double *sol, double *lagr, double *crval,
	double *amat, double *bvec, int *fdamat, int *q,
	int *meq, int *iact, int *nact, int *iter,
	double *work, int *ierr
);

extern void aind(
	int *ind, int *m, int *q, int *n, int *ok
);

Handle<Value> VException(const char *msg) {
  HandleScope scope;
  return ThrowException(Exception::Error(String::New(msg)));
}

// lalg functions
class Vector {
private:
  double* elements;
  int length;

public:
  Vector(int length) {
    this->length = length;
    this->elements = new double[length];
  }
  ~Vector() {
    delete[] elements;
  }

  double Get(int i) {
    return elements[i];
  }
  double* Elements() {
    return elements;
  }
  int Length() {
    return length;
  }
  double Set(int i, double v) {
    elements[i] = v;
  }

  static Vector* FromJavascript(Handle<Array> value) {
    HandleScope scope;

    if (!value->IsArray()) {
      return NULL;
    }
    Handle<Array> arr = Local<Array>(Array::Cast(*value));

    Vector* vec = new Vector(arr->Length());
    for (int i=0; i<arr->Length(); i++) {
      Handle<Value> v = arr->Get(Integer::New(i));
      if (v->IsNumber()) {
        vec->Set(i, v->ToNumber()->Value());
      }
    }
    return vec;
  }
};
class Matrix {
private:
  double* elements;
  int rows;
  int cols;

public:
  Matrix(int rows, int cols) {
    this->rows = rows;
    this->cols = cols;
    this->elements = new double[rows*cols];
  }
  ~Matrix() {
    delete[] elements;
  }

  double Get(int row, int col) {
    return elements[row * cols + col];
  }
  Vector* GetColumn(int col) {
    Vector* v = new Vector(rows);
    for (int i=0; i<this->rows; i++) {
      v->Set(i, Get(i, col));
    }
    return v;
  }
  Vector* GetRow(int row) {
    Vector* v = new Vector(cols);
    for (int i=0; i<this->cols; i++) {
      v->Set(i, Get(row, i));
    }
    return v;
  }
  int Columns() {
    return cols;
  }
  double* Elements() {
    return elements;
  }
  int Rows() {
    return rows;
  }
  void Set(int row, int col, double value) {
    elements[row * cols + col] = value;
  }
  static Matrix* FromJavascript(Handle<Array> value) {
    HandleScope scope;

    int cols = 0;
    int rows = value->Length();
    for (int i=0; i<rows; i++) {
      Handle<Value> rv = value->Get(Integer::New(i));
      if (!rv->IsArray()) {
        return NULL;
      }

      Handle<Array> arr = Local<Array>(Array::Cast(*rv));
      if (arr->Length() > cols) {
        cols = arr->Length();
      }
    }
    Matrix* m = new Matrix(rows, cols);
    for (int i=0; i<rows; i++) {
      Handle<Value> rv = value->Get(Integer::New(i));
      Handle<Array> arr = Local<Array>(Array::Cast(*rv));
      for (int j=0; j<arr->Length(); j++) {
        Handle<Value> v = arr->Get(Integer::New(j));
        if (v->IsNumber()) {
          m->Set(i, j, v->ToNumber()->Value());
        }
      }
    }
    return m;
  }
};

Handle<Value> Solve(Matrix* D, Vector* d, Matrix* A1, Vector* b1, Matrix* A2, Vector* b2) {
  HandleScope scope;

  Matrix* A = new Matrix(
    A1->Rows() + A2->Rows(),
    A2->Columns() > A1->Columns() ? A2->Columns() : A1->Columns()
  );
  Vector* b = new Vector(b1->Length() + b2->Length());
  int meq = b1->Length();

  for (int i=0; i<A1->Rows(); i++) {
    for (int j=0; j<A1->Columns(); j++) {
      A->Set(i, j, A1->Get(i, j));
    }
  }
  for (int i=0; i<A2->Rows(); i++) {
    for (int j=0; j<A2->Columns(); j++) {
      A->Set(meq+i, j, A2->Get(i, j));
    }
  }
  for (int i=0; i<b1->Length(); i++) {
    b->Set(i, b1->Get(i));
  }
  for (int i=0; i<b2->Length(); i++) {
    b->Set(meq + i, b2->Get(i));
  }

  int error = 0;
  int n = D->Rows();
  int q = A->Rows();

  Handle<Value> result;

  if (n != D->Columns()) {
    result = VException("The D matrix must be symmetric");
  } else if (n != d->Length()) {
    result = VException("The D matrix and the d vector are incompatible");
  } else if (n != A->Columns()) {
    result = VException("The A matrix and the d vector are incompatible");
  } else if (q != b->Length()) {
    result = VException("The A matrix and the b vector are incompatible");
  } else {
    int* iact = new int[q];
    int nact = 0;
    int r = n;
    if (r > q) {
      r = q;
    }
    Vector* sol = new Vector(n);
    Vector* lagr = new Vector(q);
    double crval = 0;
    Vector* work = new Vector(2 * n + r * (r + 5) / 2 + 2 * q + 1);
    int* iter = new int[2];

    qpgen2_(
      D->Elements(),
      d->Elements(),
      &n,
      &n,
      sol->Elements(),
      lagr->Elements(),
      &crval,
      A->Elements(),
      b->Elements(),
      &n,
      &q,
      &meq,
      iact,
      &nact,
      iter,
      work->Elements(),
      &error
    );

    delete[] iter;
    delete work;
    delete lagr;
    delete sol;
    delete[] iact;
  }

  delete b;
  delete A;

  return result;
}

Handle<Value> Solve(const Arguments &args) {
  HandleScope scope;

  if (args.Length() != 6) {
    return VException("Expected 6 arguments: D, d, A1, b1, A2, b2");
  }
  // Check argument types
  if (!args[0]->IsArray()) {
    return VException("D must be an array");
  }
  if (!args[1]->IsArray()) {
    return VException("d must be an array");
  }
  if (!args[2]->IsArray()) {
    return VException("A1 must be an array");
  }
  if (!args[3]->IsArray()) {
    return VException("b1 must be an array");
  }
  if (!args[4]->IsArray()) {
    return VException("A2 must be an array");
  }
  if (!args[5]->IsArray()) {
    return VException("b2 must be an array");
  }

  Handle<Array> arg0 = Local<Array>(Array::Cast(*args[0]));
  Handle<Array> arg1 = Local<Array>(Array::Cast(*args[1]));
  Handle<Array> arg2 = Local<Array>(Array::Cast(*args[2]));
  Handle<Array> arg3 = Local<Array>(Array::Cast(*args[3]));
  Handle<Array> arg4 = Local<Array>(Array::Cast(*args[4]));
  Handle<Array> arg5 = Local<Array>(Array::Cast(*args[5]));

  Matrix* D = Matrix::FromJavascript(arg0);
  Vector* d = Vector::FromJavascript(arg1);
  Matrix* A1 = Matrix::FromJavascript(arg2);
  Vector* b1 = Vector::FromJavascript(arg3);
  Matrix* A2 = Matrix::FromJavascript(arg4);
  Vector* b2 = Vector::FromJavascript(arg5);;

  Handle<Value> result = Solve(D, d, A1, b1, A2, b2);

  delete b2;
  delete A2;
  delete b1;
  delete A1;
  delete d;
  delete D;

  return scope.Close(result);
}

extern "C" void init(Handle<Object> target) {
  HandleScope scope;
  NODE_SET_METHOD(target, "solve", Solve);
}