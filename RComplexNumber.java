package org.rosuda.JRI;

/** class that represents a complex number. A complex number is a remainder and
 * multiplier. In R, it is represented as remainder + multiplier i.
 */
public class RComplexNumber
{
	private double multiplier;
	private double remainder;
	
	/** default constructor.
	 * @param multiplier the multiplier of the number (the multiplier * i).
	 * @param remainder the remainder of the number (remainder + xi).
	 */
	public RComplexNumber(double multiplier, double remainder)
	{
		this.multiplier = multiplier;
		this.remainder = remainder;
	}
	
	/** get the number's multiplier. */
	public double getMultiplier()
	{
		return multiplier;
	}
	
	/** get the number's remainder. */
	public double getReminder()
	{
		return remainder;
	}
	
	/** get the string representation of the complex number. You cannot adjust
	 * the precision via this method. Use a DecimalFormatter or String.format
	 * if you would like more control of the number's format.
	 * @return a string with the following format: r+mi
	 */
	public String toString()
	{
		String tail = "";
		if(multiplier > 0)
			tail = "+" + multiplier + "i";
		else if(multiplier < 0)
			tail = multiplier + "i";
		
		return remainder + tail;
	}
}