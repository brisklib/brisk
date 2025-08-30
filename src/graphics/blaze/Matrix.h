
#pragma once


#include "Utils.h"

namespace Blaze {

/**
 * Describes how complex 3x2 matrix is.
 */
enum class MatrixComplexity : uint8_t {

    /**
     * Identity matrix. Transforming point by this matrix will result in
     * identical point.
     */
    Identity = 0,


    /**
     * Matrix only contains translation and no scale or other components.
     */
    TranslationOnly,


    /**
     * Matrix only contains scale, but no translation or other components.
     */
    ScaleOnly,


    /**
     * Matrix contains a combination of translation and scale.
     */
    TranslationScale,


    /**
     * Matrix potentially contains a combination of scale, translation,
     * rotation and skew.
     */
    Complex
};


/**
 * A class encapsulating a 3x2 matrix.
 */
class Matrix final {
public:
    /**
     * Pre-constructed identity matrix.
     */
    static const Matrix Identity;


    /**
     * Constructs identity 3x2 matrix.
     */
    Matrix();


    /**
     * Constructs translation matrix with given position.
     */
    explicit Matrix(const FloatPoint &translation);


    /**
     * Constructs matrix as product of two given matrices.
     */
    Matrix(const Matrix &matrix1, const Matrix &matrix2);


    /**
     * Constructs a copy of a given 3x2 matrix.
     */
    Matrix(const Matrix &matrix);


    /**
     * Contructs 3x2 matrix from given components.
     */
    Matrix(const Float m11, const Float m12, const Float m21,
        const Float m22, const Float m31, const Float m32);


    /**
     * Creates a translation matrix from the given vector.
     */
    static Matrix CreateTranslation(const FloatPoint &translation);


    /**
     * Creates a translation matrix from the given x and y values.
     */
    static Matrix CreateTranslation(const Float x, const Float y);


    /**
     * Creates a scale matrix from the given vector.
     */
    static Matrix CreateScale(const FloatPoint &scale);


    /**
     * Creates a scale matrix from the given x and y values.
     */
    static Matrix CreateScale(const Float x, const Float y);


    /**
     * Creates scale matrix that from a single scale value which is used as
     * scale factor for both x and y.
     */
    static Matrix CreateScale(const Float scale);


    /**
     * Creates a skew matrix from the given angles in degrees.
     */
    static Matrix CreateSkew(const Float degreesX, const Float degreesY);


    /**
     * Creates a 3x2 rotation matrix using the given rotation in degrees.
     */
    static Matrix CreateRotation(const Float degrees);


    /**
     * Linearly interpolates from matrix1 to matrix2, based on the third
     * parameter.
     */
    static Matrix Lerp(
        const Matrix &matrix1, const Matrix &matrix2, const Float t);


    /**
     * Returns whether the matrix is the identity matrix.
     */
    bool IsIdentity() const;


    /**
     * Calculates the determinant for this matrix.
     */
    Float GetDeterminant() const;


    /**
     * Attempts to invert this matrix. If the operation succeeds, the inverted
     * matrix is stored in the result parameter and true is returned.
     * Otherwise, identity matrix is stored in the result parameter and false
     * is returned.
     */
    bool Invert(Matrix &result) const;


    /**
     * Attempts to invert this matrix. If the operation succeeds, the inverted
     * matrix is returned. Otherwise, identity matrix is returned.
     */
    Matrix Inverse() const;


    /**
     * Maps given point by this matrix.
     */
    FloatPoint Map(const FloatPoint &point) const;


    /**
     * Maps given point by this matrix.
     */
    FloatPoint Map(const Float x, const Float y) const;


    /**
     * Maps given rectangle by this matrix.
     */
    FloatRect Map(const FloatRect &rect) const;


    /**
     * Maps all four corner points of a given rectangle and returns a new
     * rectangle which fully contains transformed points.
     */
    IntRect MapBoundingRect(const IntRect &rect) const;


    /**
     * Post-multiplies this matrix by a given matrix.
     */
    void PostMultiply(const Matrix &matrix);


    /**
     * Pre-multiplies this matrix by a given matrix.
     */
    void PreMultiply(const Matrix &matrix);


    /**
     * Returns M11 element of matrix.
     */
    Float M11() const;


    /**
     * Sets M11 element of matrix.
     */
    void SetM11(const Float value);


    /**
     * Returns M12 element of matrix.
     */
    Float M12() const;


    /**
     * Sets M12 element of matrix.
     */
    void SetM12(const Float value);


    /**
     * Returns M21 element of matrix.
     */
    Float M21() const;


    /**
     * Sets M21 element of matrix.
     */
    void SetM21(const Float value);


    /**
     * Returns M22 element of matrix.
     */
    Float M22() const;


    /**
     * Sets M22 element of matrix.
     */
    void SetM22(const Float value);


    /**
     * Returns M31 element of matrix.
     */
    Float M31() const;


    /**
     * Sets M31 element of matrix.
     */
    void SetM31(const Float value);


    /**
     * Returns M32 element of matrix.
     */
    Float M32() const;


    /**
     * Sets M32 element of matrix.
     */
    void SetM32(const Float value);


    /**
     * Returns true if this matrix contains the same values as a given matrix.
     */
    bool IsEqual(const Matrix &matrix) const;


    /**
     * Returns translation components of this matrix as point.
     */
    FloatPoint GetTranslation() const;


    /**
     * Pre-multiplies this matrix by translation matrix constructed with given
     * translation values.
     */
    void PreTranslate(const FloatPoint &translation);


    /**
     * Post-multiplies this matrix by translation matrix constructed with
     * given translation values.
     */
    void PostTranslate(const FloatPoint &translation);


    /**
     * Pre-multiplies this matrix by translation matrix constructed with given
     * translation values.
     */
    void PreTranslate(const Float x, const Float y);


    /**
     * Post-multiplies this matrix by translation matrix constructed with
     * given translation values.
     */
    void PostTranslate(const Float x, const Float y);


    /**
     * Pre-multiplies this matrix by scale matrix constructed with given scale
     * values.
     */
    void PreScale(const FloatPoint &scale);


    /**
     * Post-multiplies this matrix by scale matrix constructed with given
     * scale values.
     */
    void PostScale(const FloatPoint &scale);


    /**
     * Pre-multiplies this matrix by scale matrix constructed with given scale
     * values.
     */
    void PreScale(const Float x, const Float y);


    /**
     * Post-multiplies this matrix by scale matrix constructed with given
     * scale values.
     */
    void PostScale(const Float x, const Float y);


    /**
     * Pre-multiplies this matrix by scale matrix constructed with given scale
     * value.
     */
    void PreScale(const Float scale);


    /**
     * Post-multiplies this matrix by scale matrix constructed with given
     * scale value.
     */
    void PostScale(const Float scale);


    /**
     * Pre-multiplies this matrix with rotation matrix constructed with given
     * rotation in degrees.
     */
    void PreRotate(const Float degrees);


    /**
     * Post-multiplies this matrix with rotation matrix constructed with given
     * rotation in degrees.
     */
    void PostRotate(const Float degrees);


    /**
     * Determine matrix complexity.
     */
    MatrixComplexity DetermineComplexity() const;


    bool operator==(const Matrix &matrix) const;
    bool operator!=(const Matrix &matrix) const;

private:
    Float m[3][2];
};


inline Matrix::Matrix() {
    m[0][0] = 1;
    m[0][1] = 0;
    m[1][0] = 0;
    m[1][1] = 1;
    m[2][0] = 0;
    m[2][1] = 0;
}


inline Matrix::Matrix(const Matrix &matrix) {
    m[0][0] = matrix.m[0][0];
    m[0][1] = matrix.m[0][1];
    m[1][0] = matrix.m[1][0];
    m[1][1] = matrix.m[1][1];
    m[2][0] = matrix.m[2][0];
    m[2][1] = matrix.m[2][1];
}


inline Matrix::Matrix(const Float m11, const Float m12, const Float m21,
    const Float m22, const Float m31, const Float m32) {
    m[0][0] = m11;
    m[0][1] = m12;
    m[1][0] = m21;
    m[1][1] = m22;
    m[2][0] = m31;
    m[2][1] = m32;
}


inline Matrix Matrix::CreateTranslation(const FloatPoint &translation) {
    return CreateTranslation(translation.X, translation.Y);
}


inline Matrix Matrix::CreateTranslation(const Float x, const Float y) {
    return Matrix(1, 0, 0, 1, x, y);
}


inline Matrix Matrix::CreateScale(const FloatPoint &scale) {
    return CreateScale(scale.X, scale.Y);
}


inline Matrix Matrix::CreateScale(const Float x, const Float y) {
    return Matrix(x, 0, 0, y, 0, 0);
}


inline Matrix Matrix::CreateScale(const Float scale) {
    return Matrix(scale, 0, 0, scale, 0, 0);
}


inline Matrix Matrix::CreateSkew(const Float degreesX, const Float degreesY) {
    if (FuzzyIsZero(degreesX) && FuzzyIsZero(degreesY)) {
        return Matrix::Identity;
    }

    const Float xTan = Tan(Deg2Rad(degreesX));
    const Float yTan = Tan(Deg2Rad(degreesY));

    return Matrix(1.0, yTan, xTan, 1.0, 0.0, 0.0);
}


inline Float Matrix::GetDeterminant() const {
    return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}


inline FloatPoint Matrix::Map(const FloatPoint &point) const {
    return FloatPoint{ m[0][0] * point.X + m[1][0] * point.Y + m[2][0],
        m[0][1] * point.X + m[1][1] * point.Y + m[2][1] };
}


inline FloatPoint Matrix::Map(const Float x, const Float y) const {
    return FloatPoint{ m[0][0] * x + m[1][0] * y + m[2][0],
        m[0][1] * x + m[1][1] * y + m[2][1] };
}


inline Float Matrix::M11() const {
    return m[0][0];
}


inline void Matrix::SetM11(const Float value) {
    m[0][0] = value;
}


inline Float Matrix::M12() const {
    return m[0][1];
}


inline void Matrix::SetM12(const Float value) {
    m[0][1] = value;
}


inline Float Matrix::M21() const {
    return m[1][0];
}


inline void Matrix::SetM21(const Float value) {
    m[1][0] = value;
}


inline Float Matrix::M22() const {
    return m[1][1];
}


inline void Matrix::SetM22(const Float value) {
    m[1][1] = value;
}


inline Float Matrix::M31() const {
    return m[2][0];
}


inline void Matrix::SetM31(const Float value) {
    m[2][0] = value;
}


inline Float Matrix::M32() const {
    return m[2][1];
}


inline void Matrix::SetM32(const Float value) {
    m[2][1] = value;
}


inline bool Matrix::operator==(const Matrix &matrix) const {
    return IsEqual(matrix);
}


inline bool Matrix::operator!=(const Matrix &matrix) const {
    return !IsEqual(matrix);
}


inline FloatPoint Matrix::GetTranslation() const {
    return FloatPoint{ m[2][0], m[2][1] };
}

} // namespace Blaze